/*
 * Copyright (C) 2016 Swift Navigation Inc.
 * Contact: Mark Fine <mfine@swift-nav.com>
 *
 * This source is subject to the license found in the file 'LICENSE' which must
 * be be distributed together with this source. All other rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <curl/curl.h>
#include <libskylark/errno.h>
#include <libskylark/http.h>
#include <libskylark/logging.h>
#include <stdlib.h>
#include <unistd.h>

size_t publish_callback(void *p, size_t size, size_t n, void *up)
{
  if (size * n < 1)
  {
    return 0;
  }
  char buf;
  if (read(*(int *)up, &buf, 1) < 0)
  {
    log_error("publish");
    return 0;
  }
  *(char *)p = buf;
  return 1;
}

size_t subscribe_callback(void *ptr, size_t size, size_t n, void *stream)
{
  log_info("callback");
  return fwrite(ptr, size, n, (FILE *)stream);
}

void setup_broker(void)
{
  curl_global_init(CURL_GLOBAL_ALL);
}

int publish(char *uid, int fd)
{
  CURL *curl = curl_easy_init();
  if (curl == NULL)
  {
    curl_global_cleanup();
    return -GENERIC_ERROR;
  }
  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, ENCODING);
  chunk = curl_slist_append(chunk, ACCEPT_TYPE);
  chunk = curl_slist_append(chunk, CONTENT_TYPE);
  char buf[sizeof(DEFAULT_DEVICE_UID)];
  sprintf(buf, "Device-Uid: %s", uid);
  chunk = curl_slist_append(chunk, buf);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER,   chunk);
#ifdef VERBOSE
  curl_easy_setopt(curl, CURLOPT_VERBOSE,      1);
#endif
#ifdef SKIP_PEER_VERIFICATION
  // In the case that the host is using a self-signed CA.
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
  curl_easy_setopt(curl, CURLOPT_URL,          BROKER_ENDPOINT);
  curl_easy_setopt(curl, CURLOPT_PUT,          1);
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, publish_callback);
  curl_easy_setopt(curl, CURLOPT_READDATA,     &fd);
  curl_easy_setopt(curl, CURLOPT_USERAGENT,    USER_AGENT);
  int res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    log_error(curl_easy_strerror(res));
    return -11;
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return NO_ERROR;
}

int subscribe(char *uid, int fd)
{
  CURL *curl = curl_easy_init();
  if (curl == NULL)
  {
    curl_global_cleanup();
    return -GENERIC_ERROR;
  }
  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, ENCODING);
  chunk = curl_slist_append(chunk, ACCEPT_TYPE);
  char buf[sizeof(DEFAULT_DEVICE_UID)];
  sprintf(buf, "Device-Uid: %s", uid);
  chunk = curl_slist_append(chunk, buf);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER,   chunk);
#ifdef VERBOSE
  curl_easy_setopt(curl, CURLOPT_VERBOSE,      1);
#endif
#ifdef SKIP_PEER_VERIFICATION
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
  curl_easy_setopt(curl, CURLOPT_URL,           BROKER_ENDPOINT);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, subscribe_callback);
  curl_easy_setopt(curl, CURLOPT_USERAGENT,     USER_AGENT);
  static const char *pagefilename = "page.out";
  FILE* page = fopen(pagefilename, "wb");
  log_info("here");
  if (page == NULL) {
    curl_global_cleanup();
    return -GENERIC_ERROR;
  }
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &page);
  int res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    log_error(curl_easy_strerror(res));
    return -11;
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  fclose(page);
  return fd;
}
