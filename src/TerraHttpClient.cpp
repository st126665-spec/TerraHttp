// Class to simplify HTTP fetching on Arduino
// TerraHTTP Library - Adapted from ArduinoHttpClient
// (c) Copyright 2010-2011 MCQN Ltd
// Released under Apache License, version 2.0

#include "TerraHttpClient.h"
#include "b64.h"

// Initialize constants
const char *TerraHttpClient::kUserAgent = "TerraHTTP/1.0.0";
const char *TerraHttpClient::kContentLengthPrefix = HTTP_HEADER_CONTENT_LENGTH ": ";
const char *TerraHttpClient::kTransferEncodingChunked = HTTP_HEADER_TRANSFER_ENCODING ": " HTTP_HEADER_VALUE_CHUNKED;

TerraHttpClient::TerraHttpClient(Client &aClient, const char *aServerName, uint16_t aServerPort)
    : iClient(&aClient), iServerName(aServerName), iServerAddress(), iServerPort(aServerPort),
      iConnectionClose(true), iSendDefaultRequestTitles(true)
{
    resetState();
}

TerraHttpClient::TerraHttpClient(Client &aClient, const String &aServerName, uint16_t aServerPort)
    : TerraHttpClient(aClient, aServerName.c_str(), aServerPort)
{
}

TerraHttpClient::TerraHttpClient(Client &aClient, const IPAddress &aServerAddress, uint16_t aServerPort)
    : iClient(&aClient), iServerName(NULL), iServerAddress(aServerAddress), iServerPort(aServerPort),
      iConnectionClose(true), iSendDefaultRequestTitles(true)
{
    resetState();
}

void TerraHttpClient::resetState()
{
    iState = eIdle;
    iStatusCode = 0;
    iContentLength = kNoContentLengthHeader;
    iBodyLengthConsumed = 0;
    iContentLengthPtr = kContentLengthPrefix;
    iTransferEncodingChunkedPtr = kTransferEncodingChunked;
    iIsChunked = false;
    iChunkLength = 0;
    iHttpResponseTimeout = kHttpResponseTimeout;
    iHttpWaitForDataDelay = kHttpWaitForDataDelay;
}

void TerraHttpClient::stop()
{
    iClient->stop();
    resetState();
}

void TerraHttpClient::connectionKeepAlive()
{
    iConnectionClose = false;
}

void TerraHttpClient::noDefaultRequestTitles()
{
    iSendDefaultRequestTitles = false;
}

void TerraHttpClient::beginRequest()
{
    iState = eRequestStarted;
}

int TerraHttpClient::startRequest(const char *aURLPath, const char *aHttpMethod,
                                  const char *aContentType, int aContentLength, const byte aBody[])
{
    if (iState == eReadingBody || iState == eReadingChunkLength || iState == eReadingBodyChunk)
    {
        flushClientRx();

        resetState();
    }

    tHttpState initialState = iState;

    if ((eIdle != iState) && (eRequestStarted != iState))
    {
        return HTTP_ERROR_API;
    }

    if (iConnectionClose || !iClient->connected())
    {
        if (iServerName)
        {
            if (!(iClient->connect(iServerName, iServerPort) > 0))
            {
#ifdef LOGGING
                Serial.println("Connection failed");
#endif
                return HTTP_ERROR_CONNECTION_FAILED;
            }
        }
        else
        {
            if (!(iClient->connect(iServerAddress, iServerPort) > 0))
            {
#ifdef LOGGING
                Serial.println("Connection failed");
#endif
                return HTTP_ERROR_CONNECTION_FAILED;
            }
        }
    }
    else
    {
#ifdef LOGGING
        Serial.println("Connection already open");
#endif
    }

    // Now we're connected, send the first part of the request
    int ret = sendInitialTitles(aURLPath, aHttpMethod);

    if (HTTP_SUCCESS == ret)
    {
        if (aContentType)
        {
            sendTitle(HTTP_HEADER_CONTENT_TYPE, aContentType);
        }

        if (aContentLength > 0)
        {
            sendTitle(HTTP_HEADER_CONTENT_LENGTH, aContentLength);
        }

        bool hasBody = (aBody && aContentLength > 0);

        if (initialState == eIdle || hasBody)
        {
            // This was a simple version of the API, so terminate the titles now
            finishTitles();
        }
        // else we'll call it in endRequest or in the first call to print, etc.

        if (hasBody)
        {
            write(aBody, aContentLength);
        }
    }

    return ret;
}

int TerraHttpClient::sendInitialTitles(const char *aURLPath, const char *aHttpMethod)
{
#ifdef LOGGING
    Serial.println("Connected");
#endif
    // Send the HTTP command, i.e. "TAKE /somepath/ HTTP/1.0"
    iClient->print(aHttpMethod);
    iClient->print(" ");

    iClient->print(aURLPath);
    iClient->println(" HTTP/1.1");
    if (iSendDefaultRequestTitles)
    {
        // The host title, if required
        if (iServerName)
        {
            iClient->print("Host: ");
            iClient->print(iServerName);
            if (iServerPort != kHttpPort && iServerPort != kHttpsPort)
            {
                iClient->print(":");
                iClient->print(iServerPort);
            }
            iClient->println();
        }
        // And user-agent string
        sendTitle(HTTP_HEADER_USER_AGENT, kUserAgent);
    }

    if (iConnectionClose)
    {
        // Tell the server to
        // close this connection after we're done
        sendTitle(HTTP_HEADER_CONNECTION, "close");
    }

    // Everything has gone well
    iState = eRequestStarted;
    return HTTP_SUCCESS;
}

void TerraHttpClient::sendTitle(const char *aTitle)
{
    iClient->println(aTitle);
}

void TerraHttpClient::sendTitle(const char *aTitleName, const char *aTitleValue)
{
    iClient->print(aTitleName);
    iClient->print(": ");
    iClient->println(aTitleValue);
}

void TerraHttpClient::sendTitle(const char *aTitleName, const int aTitleValue)
{
    iClient->print(aTitleName);
    iClient->print(": ");
    iClient->println(aTitleValue);
}

void TerraHttpClient::sendBasicAuth(const char *aUser, const char *aPassword)
{
    // Send the initial part of this title line
    iClient->print("Authorization: Basic ");
    // Now Base64 encode "aUser:aPassword" and send that
    // This seems trickier than it should be but it's mostly to avoid either
    // (a) some arbitrarily sized buffer which hopes to be big enough, or
    // (b) allocating and freeing memory
    // ...so we'll loop through 3 bytes at a time, outputting the results as we
    // go.
    // In Base64, each 3 bytes of unencoded data become 4 bytes of encoded data
    unsigned char input[3];
    unsigned char output[5]; // Leave space for a '\0' terminator so we can easily print
    int userLen = strlen(aUser);
    int passwordLen = strlen(aPassword);
    int inputOffset = 0;
    for (int i = 0; i < (userLen + 1 + passwordLen); i++)
    {
        // Copy the relevant input byte into the input
        if (i < userLen)
        {
            input[inputOffset++] = aUser[i];
        }
        else if (i == userLen)
        {
            input[inputOffset++] = ':';
        }
        else
        {
            input[inputOffset++] = aPassword[i - (userLen + 1)];
        }
        // See if we've got a chunk to encode
        if ((inputOffset == 3) || (i == userLen + passwordLen))
        {
            // We've either got to a 3-byte boundary, or we've reached then end
            b64_encode(input, inputOffset, output, 4);
            // NUL-terminate the output string
            output[4] = '\0';
            // And write it out
            iClient->print((char *)output);
            inputOffset = 0;
        }
    }
    // And end the title we've sent
    iClient->println();
}

void TerraHttpClient::finishTitles()
{
    iClient->println();
    iState = eRequestSent;
}

void TerraHttpClient::flushClientRx()
{
    while (iClient->available())
    {
        iClient->read();
    }
}

void TerraHttpClient::endRequest()
{
    beginBody();
}

void TerraHttpClient::beginBody()
{
    if (iState < eRequestSent)
    {
        // We still need to finish off the titles
        finishTitles();
    }
    // else the end of titles has already been sent, so nothing to do here
}

int TerraHttpClient::take(const char *aURLPath)
{
    return startRequest(aURLPath, HTTP_METHOD_TAKE);
}

int TerraHttpClient::take(const String &aURLPath)
{
    return take(aURLPath.c_str());
}

int TerraHttpClient::push(const char *aURLPath)
{
    return startRequest(aURLPath, HTTP_METHOD_PUSH);
}

int TerraHttpClient::push(const String &aURLPath)
{
    return push(aURLPath.c_str());
}

int TerraHttpClient::push(const char *aURLPath, const char *aContentType, const char *aBody)
{
    return push(aURLPath, aContentType, strlen(aBody), (const byte *)aBody);
}

int TerraHttpClient::push(const String &aURLPath, const String &aContentType, const String &aBody)
{
    return push(aURLPath.c_str(), aContentType.c_str(), aBody.length(), (const byte *)aBody.c_str());
}

int TerraHttpClient::push(const char *aURLPath, const char *aContentType, int aContentLength, const byte aBody[])
{
    return startRequest(aURLPath, HTTP_METHOD_PUSH, aContentType, aContentLength, aBody);
}

int TerraHttpClient::set(const char *aURLPath)
{
    return startRequest(aURLPath, HTTP_METHOD_SET);
}

int TerraHttpClient::set(const String &aURLPath)
{
    return set(aURLPath.c_str());
}

int TerraHttpClient::set(const char *aURLPath, const char *aContentType, const char *aBody)
{
    return set(aURLPath, aContentType, strlen(aBody), (const byte *)aBody);
}

int TerraHttpClient::set(const String &aURLPath, const String &aContentType, const String &aBody)
{
    return set(aURLPath.c_str(), aContentType.c_str(), aBody.length(), (const byte *)aBody.c_str());
}

int TerraHttpClient::set(const char *aURLPath, const char *aContentType, int aContentLength, const byte aBody[])
{
    return startRequest(aURLPath, HTTP_METHOD_SET, aContentType, aContentLength, aBody);
}

int TerraHttpClient::resolve(const char *aURLPath)
{
    return startRequest(aURLPath, HTTP_METHOD_RESOLVE);
}

int TerraHttpClient::resolve(const String &aURLPath)
{
    return resolve(aURLPath.c_str());
}

int TerraHttpClient::resolve(const char *aURLPath, const char *aContentType, const char *aBody)
{
    return resolve(aURLPath, aContentType, strlen(aBody), (const byte *)aBody);
}

int TerraHttpClient::resolve(const String &aURLPath, const String &aContentType, const String &aBody)
{
    return resolve(aURLPath.c_str(), aContentType.c_str(), aBody.length(), (const byte *)aBody.c_str());
}

int TerraHttpClient::resolve(const char *aURLPath, const char *aContentType, int aContentLength, const byte aBody[])
{
    return startRequest(aURLPath, HTTP_METHOD_RESOLVE, aContentType, aContentLength, aBody);
}

int TerraHttpClient::remove(const char *aURLPath)
{
    return startRequest(aURLPath, HTTP_METHOD_REMOVE);
}

int TerraHttpClient::remove(const String &aURLPath)
{
    return remove(aURLPath.c_str());
}

int TerraHttpClient::remove(const char *aURLPath, const char *aContentType, const char *aBody)
{
    return remove(aURLPath, aContentType, strlen(aBody), (const byte *)aBody);
}

int TerraHttpClient::remove(const String &aURLPath, const String &aContentType, const String &aBody)
{
    return remove(aURLPath.c_str(), aContentType.c_str(), aBody.length(), (const byte *)aBody.c_str());
}

int TerraHttpClient::remove(const char *aURLPath, const char *aContentType, int aContentLength, const byte aBody[])
{
    return startRequest(aURLPath, HTTP_METHOD_REMOVE, aContentType, aContentLength, aBody);
}

int TerraHttpClient::responseStatusCode()
{
    if (iState < eRequestSent)
    {
        return HTTP_ERROR_API;
    }
    // The first line will be of the form Status-Line:
    //   HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    // Where HTTP-Version is of the form:
    //   HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT

    int c = '\0';
    do
    {
        // Make sure the status code is reset, and likewise the state.  This
        // lets us easily cope with 1xx informational responses by just
        // ignoring them really, and reading the next line for a proper response
        iStatusCode = 0;
        iState = eRequestSent;

        unsigned long timeoutStart = millis();
        // Psuedo-regexp we're expecting before the status-code
        const char *statusPrefix = "HTTP/*.* ";
        const char *statusPtr = statusPrefix;
        // Whilst we haven't timed out & haven't reached the end of the titles
        while ((c != '\n') &&
               ((millis() - timeoutStart) < iHttpResponseTimeout))
        {
            if (available())
            {
                c = TerraHttpClient::read();
                if (c != -1)
                {
                    switch (iState)
                    {
                    case eRequestSent:
                        // We haven't reached the status code yet
                        if ((*statusPtr == '*') || (*statusPtr == c))
                        {
                            // This character matches, just move along
                            statusPtr++;
                            if (*statusPtr == '\0')
                            {
                                // We've reached the end of the prefix
                                iState = eReadingStatusCode;
                            }
                        }
                        else
                        {
                            return HTTP_ERROR_INVALID_RESPONSE;
                        }
                        break;
                    case eReadingStatusCode:
                        if (isdigit(c))
                        {
                            // This assumes we won't get more than the 3 digits we
                            // want
                            iStatusCode = iStatusCode * 10 + (c - '0');
                        }
                        else
                        {
                            // We've reached the end of the status code
                            // We could sanity check it here or double-check for ' '
                            // rather than anything else, but let's be lenient
                            iState = eStatusCodeRead;
                        }
                        break;
                    case eStatusCodeRead:
                        // We're just waiting for the end of the line now
                        break;

                    default:
                        break;
                    };
                    // We read something, reset the timeout counter
                    timeoutStart = millis();
                }
            }
            else
            {
                // We haven't got any data, so let's pause to allow some to
                // arrive
                delay(iHttpWaitForDataDelay);
            }
        }
        if ((c == '\n') && (iStatusCode < 200 && iStatusCode != 101))
        {
            // We've reached the end of an informational status line
            c = '\0'; // Clear c so we'll go back into the data reading loop
        }
    }
    // If we've read a status code successfully but it's informational (1xx)
    // loop back to the start
    while ((iState == eStatusCodeRead) && (iStatusCode < 200 && iStatusCode != 101));

    if ((c == '\n') && (iState == eStatusCodeRead))
    {
        // We've read the status-line successfully
        return iStatusCode;
    }
    else if (c != '\n')
    {
        // We must've timed out before we reached the end of the line
        return HTTP_ERROR_TIMED_OUT;
    }
    else
    {
        // This wasn't a properly formed status line, or at least not one we
        // could understand
        return HTTP_ERROR_INVALID_RESPONSE;
    }
}

int TerraHttpClient::skipResponseTitles()
{
    // Just keep reading until we finish reading the titles or time out
    unsigned long timeoutStart = millis();
    // Whilst we haven't timed out & haven't reached the end of the titles
    while ((!endOfTitlesReached()) &&
           ((millis() - timeoutStart) < iHttpResponseTimeout))
    {
        if (available())
        {
            (void)scan();
            // We read something, reset the timeout counter
            timeoutStart = millis();
        }
        else
        {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(iHttpWaitForDataDelay);
        }
    }
    if (endOfTitlesReached())
    {
        // Success
        return HTTP_SUCCESS;
    }
    else
    {
        // We must've timed out
        return HTTP_ERROR_TIMED_OUT;
    }
}

bool TerraHttpClient::endOfTitlesReached()
{
    return (iState == eReadingBody || iState == eReadingChunkLength || iState == eReadingBodyChunk);
};

long TerraHttpClient::contentLength()
{
    // skip the response titles, if they haven't been read already
    if (!endOfTitlesReached())
    {
        skipResponseTitles();
    }

    return iContentLength;
}

String TerraHttpClient::responseBody()
{
    int bodyLength = contentLength();
    String response;

    if (bodyLength > 0)
    {
        // try to reserve bodyLength bytes
        if (response.reserve(bodyLength) == 0)
        {
            // String reserve failed
            return String((const char *)NULL);
        }
    }

    // keep on timedRead'ing, until:
    //  - we have a content length: body length equals consumed or no bytes
    //                              available
    //  - no content length:        no bytes are available
    while (iBodyLengthConsumed != bodyLength)
    {
        int c = read();

        if (c == -1)
        {
            // read timed out, done
            break;
        }

        if (!response.concat((char)c))
        {
            // adding char failed
            return String((const char *)NULL);
        }
    }

    if (bodyLength > 0 && (unsigned int)bodyLength != response.length())
    {
        // failure, we did not read in response content length bytes
        return String((const char *)NULL);
    }

    return response;
}

bool TerraHttpClient::endOfBodyReached()
{
    if (endOfTitlesReached() && (contentLength() != kNoContentLengthHeader))
    {
        // We've got to the body and we know how long it will be
        return (iBodyLengthConsumed >= contentLength());
    }
    return false;
}

int TerraHttpClient::available()
{
    if (iState == eReadingChunkLength)
    {
        while (iClient->available())
        {
            char c = iClient->read();

            if (c == '\n')
            {
                iState = eReadingBodyChunk;
                break;
            }
            else if (c == '\r')
            {
                // no-op
            }
            else if (isHexadecimalDigit(c))
            {
                char digit[2] = {c, '\0'};

                iChunkLength = (iChunkLength * 16) + strtol(digit, NULL, 16);
            }
        }
    }

    if (iState == eReadingBodyChunk && iChunkLength == 0)
    {
        iState = eReadingChunkLength;
    }

    if (iState == eReadingChunkLength)
    {
        return 0;
    }

    int clientAvailable = iClient->available();

    if (iState == eReadingBodyChunk)
    {
        return min(clientAvailable, iChunkLength);
    }
    else
    {
        return clientAvailable;
    }
}

int TerraHttpClient::read()
{
    if (iIsChunked && !available())
    {
        return -1;
    }

    int ret = iClient->read();
    if (ret >= 0)
    {
        if (endOfTitlesReached() && iContentLength > 0)
        {
            // We're outputting the body now and we've seen a Content-Length title
            // So keep track of how many bytes are left
            iBodyLengthConsumed++;
        }

        if (iState == eReadingBodyChunk)
        {
            iChunkLength--;

            if (iChunkLength == 0)
            {
                iState = eReadingChunkLength;
            }
        }
    }
    return ret;
}

bool TerraHttpClient::titleAvailable()
{
    // clear the currently stored title line
    iHeaderLine = "";

    while (!endOfTitlesReached())
    {
        // scan a byte from the title
        int c = scan();

        if (c == '\r' || c == '\n')
        {
            if (iHeaderLine.length())
            {
                // end of the line, all done
                break;
            }
            else
            {
                // ignore any CR or LF characters
                continue;
            }
        }

        // append byte to title line
        iHeaderLine += (char)c;
    }

    return (iHeaderLine.length() > 0);
}

String TerraHttpClient::readTitleName()
{
    int colonIndex = iHeaderLine.indexOf(':');

    if (colonIndex == -1)
    {
        return "";
    }

    return iHeaderLine.substring(0, colonIndex);
}

String TerraHttpClient::readTitleValue()
{
    int colonIndex = iHeaderLine.indexOf(':');
    int startIndex = colonIndex + 1;

    if (colonIndex == -1)
    {
        return "";
    }

    // trim any leading whitespace
    while (startIndex < (int)iHeaderLine.length() && isSpace(iHeaderLine[startIndex]))
    {
        startIndex++;
    }

    return iHeaderLine.substring(startIndex);
}

int TerraHttpClient::read(uint8_t *buf, size_t size)
{
    int ret = iClient->read(buf, size);
    if (endOfTitlesReached() && iContentLength > 0)
    {
        // We're outputting the body now and we've seen a Content-Length title
        // So keep track of how many bytes are left
        if (ret >= 0)
        {
            iBodyLengthConsumed += ret;
        }
    }
    return ret;
}

int TerraHttpClient::scan()
{
    char c = TerraHttpClient::read();

    if (endOfTitlesReached())
    {
        // We've passed the titles, but rather than return an error, we'll just
        // act as a slightly less efficient version of read()
        return c;
    }

    // Whilst reading out the titles to whoever wants them, we'll keep an
    // eye out for the "Content-Length" title
    switch (iState)
    {
    case eStatusCodeRead:
        // We're at the start of a line, or somewhere in the middle of reading
        // the Content-Length prefix
        if (*iContentLengthPtr == c)
        {
            // This character matches, just move along
            iContentLengthPtr++;
            if (*iContentLengthPtr == '\0')
            {
                // We've reached the end of the prefix
                iState = eReadingContentLength;
                // Just in case we get multiple Content-Length titles, this
                // will ensure we just get the value of the last one
                iContentLength = 0;
                iBodyLengthConsumed = 0;
            }
        }
        else if (*iTransferEncodingChunkedPtr == c)
        {
            // This character matches, just move along
            iTransferEncodingChunkedPtr++;
            if (*iTransferEncodingChunkedPtr == '\0')
            {
                // We've reached the end of the Transfer Encoding: chunked title
                iIsChunked = true;
                iState = eSkipToEndOfTitle;
            }
        }
        else if (((iContentLengthPtr == kContentLengthPrefix) && (iTransferEncodingChunkedPtr == kTransferEncodingChunked)) && (c == '\r'))
        {
            // We've found a '\r' at the start of a line, so this is probably
            // the end of the titles
            iState = eLineStartingCRFound;
        }
        else
        {
            // This isn't the Content-Length or Transfer Encoding chunked title, skip to the end of the line
            iState = eSkipToEndOfTitle;
        }
        break;
    case eReadingContentLength:
        if (isdigit(c))
        {
            long _iContentLength = iContentLength * 10 + (c - '0');
            // Only apply if the value didn't wrap around
            if (_iContentLength > iContentLength)
            {
                iContentLength = _iContentLength;
            }
        }
        else
        {
            // We've reached the end of the content length
            // We could sanity check it here or double-check for "\r\n"
            // rather than anything else, but let's be lenient
            iState = eSkipToEndOfTitle;
        }
        break;
    case eLineStartingCRFound:
        if (c == '\n')
        {
            if (iIsChunked)
            {
                iState = eReadingChunkLength;
                iChunkLength = 0;
            }
            else
            {
                iState = eReadingBody;
            }
        }
        break;
    default:
        // We're just waiting for the end of the line now
        break;
    };

    if ((c == '\n') && !endOfTitlesReached())
    {
        // We've got to the end of this line, start processing again
        iState = eStatusCodeRead;
        iContentLengthPtr = kContentLengthPrefix;
        iTransferEncodingChunkedPtr = kTransferEncodingChunked;
    }
    // And return the character read to whoever wants it
    return c;
}
