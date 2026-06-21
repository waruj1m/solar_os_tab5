#pragma once

#include <stddef.h>

#define SOLAR_OS_IDENTITY_DEFAULT_USER "user"
#define SOLAR_OS_IDENTITY_DEFAULT_HOSTNAME "sol"
#define SOLAR_OS_IDENTITY_USER_MAX 32
#define SOLAR_OS_IDENTITY_HOSTNAME_MAX 32

void solar_os_identity_get_user(char *buffer, size_t len);
void solar_os_identity_get_hostname(char *buffer, size_t len);
void solar_os_identity_format(char *buffer, size_t len);
