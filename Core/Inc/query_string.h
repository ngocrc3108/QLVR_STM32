#include "string.h"
#include <stdint.h>
#include "stdio.h"

typedef enum {
	GP_OK = 0,
	GP_ERR
} Get_Parameter_Status;

Get_Parameter_Status getParameter(const char* queryString, const char* parameterName, char* value);
