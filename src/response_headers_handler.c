/** **************************************************************************
 * response_haeders_handler.c
 * 
 * Copyright 2008 Bryan Ischo <bryan@ischo.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the
 *
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 ************************************************************************** **/

#include <ctype.h>
#include <string.h>
#include "response_headers_handler.h"


void response_headers_handler_initialize(ResponseHeadersHandler *handler)
{
    handler->responseProperties.requestId = 0;
    handler->responseProperties.requestId2 = 0;
    handler->responseProperties.contentType = 0;
    handler->responseProperties.contentLength = -1;
    handler->responseProperties.server = 0;
    handler->responseProperties.eTag = 0;
    handler->responseProperties.lastModified = -1;
    handler->responseProperties.metaDataCount = 0;
    handler->responseProperties.metaData = 0;
    handler->done = 0;
    string_multibuffer_initialize(handler->responsePropertyStrings);
    string_multibuffer_initialize(handler->responseMetaDataStrings);
}


void response_headers_handler_add(ResponseHeadersHandler *handler,
                                  char *header, int len)
{
    S3ResponseProperties *responseProperties = &(handler->responseProperties);
    char *end = &(header[len]);
    
    // Curl might call back the header function after the body has been
    // received, for 'chunked encoded' contents.  We don't handle this as of
    // yet, and it's not clear that it would ever be useful.
    if (handler->done) {
        return;
    }

    // If we've already filled up the response headers, ignore this data.
    // This sucks, but it shouldn't happen - S3 should not be sending back
    // really long headers.
    if (handler->responsePropertyStringsSize == 
        (sizeof(handler->responsePropertyStrings) - 1)) {
        return;
    }

    // It should not be possible to have a header line less than 3 long
    if (len < 3) {
        return;
    }

    // Skip whitespace at beginning of header; there never should be any,
    // but just to be safe
    while (isblank(*header)) {
        header++;
    }

    // The header must end in \r\n, so skip back over it, and also over any
    // trailing whitespace
    end -= 3;
    while ((end > header) && isblank(*end)) {
        end--;
    }
    if (!isblank(*end)) {
        end++;
    }

    if (end == header) {
        // totally bogus
        return;
    }

    *end = 0;
    
    // Find the colon to split the header up
    char *c = header;
    while (*c && (*c != ':')) {
        c++;
    }
    
    int namelen = c - header;

    // Now walk c past the colon
    c++;
    // Now skip whitespace to the beginning of the value
    while (isblank(*c)) {
        c++;
    }

    int valuelen = (end - c) + 1, fit;

    if (!strncmp(header, "x-amz-request-id", namelen)) {
        responseProperties->requestId = 
            string_multibuffer_current(handler->responsePropertyStrings);
        string_multibuffer_add(handler->responsePropertyStrings, c, 
                               valuelen, fit);
    }
    else if (!strncmp(header, "x-amz-id-2", namelen)) {
        responseProperties->requestId2 = 
            string_multibuffer_current(handler->responsePropertyStrings);
        string_multibuffer_add(handler->responsePropertyStrings, c, 
                               valuelen, fit);
    }
    else if (!strncmp(header, "Content-Type", namelen)) {
        responseProperties->contentType = 
            string_multibuffer_current(handler->responsePropertyStrings);
        string_multibuffer_add(handler->responsePropertyStrings, c, 
                               valuelen, fit);
    }
    else if (!strncmp(header, "Content-Length", namelen)) {
        handler->responseProperties.contentLength = 0;
        while (*c) {
            handler->responseProperties.contentLength *= 10;
            handler->responseProperties.contentLength += (*c++ - '0');
        }
    }
    else if (!strncmp(header, "Server", namelen)) {
        responseProperties->server = 
            string_multibuffer_current(handler->responsePropertyStrings);
        string_multibuffer_add(handler->responsePropertyStrings, c, 
                               valuelen, fit);
    }
    else if (!strncmp(header, "ETag", namelen)) {
        responseProperties->eTag = 
            string_multibuffer_current(handler->responsePropertyStrings);
        string_multibuffer_add(handler->responsePropertyStrings, c, 
                               valuelen, fit);
    }
    else if (!strncmp(header, "x-amz-meta-", sizeof("x-amz-meta-") - 1)) {
        // Make sure there is room for another x-amz-meta header
        if (handler->responseProperties.metaDataCount ==
            sizeof(handler->responseMetaData)) {
            return;
        }
        // Copy the name in
        char *metaName = &(header[sizeof("x-amz-meta-") - 1]);
        int metaNameLen = (namelen - (sizeof("x-amz-meta-") - 1));
        char *copiedName = 
            string_multibuffer_current(handler->responseMetaDataStrings);
        string_multibuffer_add(handler->responseMetaDataStrings, metaName,
                               metaNameLen, fit);
        if (!fit) {
            return;
        }

        // Copy the value in
        char *copiedValue = 
            string_multibuffer_current(handler->responseMetaDataStrings);
        string_multibuffer_add(handler->responseMetaDataStrings,
                               c, valuelen, fit);
        if (!fit) {
            return;
        }

        if (!handler->responseProperties.metaDataCount) {
            handler->responseProperties.metaData = 
                handler->responseMetaData;
        }

        S3NameValue *metaHeader = 
            &(handler->responseMetaData
              [handler->responseProperties.metaDataCount++]);
        metaHeader->name = copiedName;
        metaHeader->value = copiedValue;
    }
}


void response_headers_handler_done(ResponseHeadersHandler *handler, CURL *curl)
{
    // Now get the last modification time from curl, since it's easiest to let
    // curl parse it
    if (curl_easy_getinfo
        (curl, CURLINFO_FILETIME, 
         &(handler->responseProperties.lastModified)) != CURLE_OK) {
        handler->responseProperties.lastModified = -1;
    }
    
    handler->done = 1;
}
