/** **************************************************************************
 * response_haeders_handler.h
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

#ifndef RESPONSE_HEADERS_HANDLER_H
#define RESPONSE_HEADERS_HANDLER_H

#include "libs3.h"
#include "string_buffer.h"
#include "util.h"


typedef struct ResponseHeadersHandler
{
    // The structure to pass to the headers callback.  This is filled in by
    // the ResponseHeadersHandler from the headers added to it.
    S3ResponseProperties responseProperties;

    // Set to 1 after the done call has been made
    int done;

    // copied into here.  We allow 128 bytes for each header, plus \0 term.
    string_multibuffer(responsePropertyStrings, 5 * 129);

    // responseproperties.metaHeaders strings get copied into here
    string_multibuffer(responseMetaDataStrings, 
                       COMPACTED_METADATA_BUFFER_SIZE);

    // Response meta data
    S3NameValue responseMetaData[S3_MAX_METADATA_COUNT];
} ResponseHeadersHandler;


void response_headers_handler_initialize(ResponseHeadersHandler *handler);

void response_headers_handler_add(ResponseHeadersHandler *handler,
                                  char *data, int dataLen);

void response_headers_handler_done(ResponseHeadersHandler *handler, 
                                   CURL *curl);

#endif /* RESPONSE_HEADERS_HANDLER_H */
