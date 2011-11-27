/*
 * Pinguino Basic Version 0.1
 * Copyright (c) 2011, Régis Blanchot
 *
 * Pinguino Basic Engine is originally based on uBasic
 * Copyright (c) 2006, Adam Dunkels
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef __TOKENIZER_C__
#define __TOKENIZER_C__

#include <string.h>
#include <ctype.h>
//#include <stdlib.h>

#include "global.h"
//#include "pbasic.c"
#include "tokenizer.h"

char const *ptr, *nextptr;

#define MAX_NUMLEN 5

struct keyword_token
{
	char *keyword;
	int token;
};

int current_token = TOKENIZER_ERROR;

const struct keyword_token keywords[] = {
  {"on", TOKENIZER_ON},
  {"off", TOKENIZER_OFF},
  {"let", TOKENIZER_LET},
  {"print", TOKENIZER_PRINT},
  {"if", TOKENIZER_IF},
  {"then", TOKENIZER_THEN},
  {"else", TOKENIZER_ELSE},
  {"for", TOKENIZER_FOR},
  {"to", TOKENIZER_TO},
  {"next", TOKENIZER_NEXT},
  {"goto", TOKENIZER_GOTO},
  {"gosub", TOKENIZER_GOSUB},
  {"return", TOKENIZER_RETURN},
  {"call", TOKENIZER_CALL},
  {"end", TOKENIZER_END},
  {"pause", TOKENIZER_PAUSE},
  {"setpin", TOKENIZER_SETPIN},
  {"pin", TOKENIZER_PIN},
  {"toggle", TOKENIZER_TOGGLE},
  {"run", TOKENIZER_RUN},
  {"load", TOKENIZER_LOAD},
  {"list", TOKENIZER_LIST},
  {"debug", TOKENIZER_DEBUG},
  {"edit", TOKENIZER_EDIT},
  {"delete", TOKENIZER_DELETE},
  {"rem", TOKENIZER_REM},
  {"help", TOKENIZER_HELP},
  {NULL, TOKENIZER_ERROR}
};

/*---------------------------------------------------------------------------*/
int singlechar(void)
{
  if(*ptr == '\n') {
    return TOKENIZER_CR;
  } else if(*ptr == ',') {
    return TOKENIZER_COMMA;
  } else if(*ptr == ';') {
    return TOKENIZER_SEMICOLON;
  } else if(*ptr == '+') {
    return TOKENIZER_PLUS;
  } else if(*ptr == '-') {
    return TOKENIZER_MINUS;
  } else if(*ptr == '&') {
    return TOKENIZER_AND;
  } else if(*ptr == '|') {
    return TOKENIZER_OR;
  } else if(*ptr == '*') {
    return TOKENIZER_ASTR;
  } else if(*ptr == '/') {
    return TOKENIZER_SLASH;
  } else if(*ptr == '%') {
    return TOKENIZER_MOD;
  } else if(*ptr == '(') {
    return TOKENIZER_LEFTPAREN;
  } else if(*ptr == ')') {
    return TOKENIZER_RIGHTPAREN;
  } else if(*ptr == '<') {
    return TOKENIZER_LT;
  } else if(*ptr == '>') {
    return TOKENIZER_GT;
  } else if(*ptr == '=') {
    return TOKENIZER_EQ;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
char * get_name_token(char *dest, char *src)
{
	char i;

	for(i=0 ; src[i] != ' ' && src[i] != '\n' && src[i] != '\0' ; i++)
		dest[i] = src[i];
	dest[i] = '\0';
	return dest;
}
/*---------------------------------------------------------------------------*/
int get_next_token(void)
{
	struct keyword_token const *kt;
	int i;
	char buffer[MAX_STRINGLEN];

	pbasic_debug("get_next_token(): %s", get_name_token(buffer, ptr));

	if(*ptr == 0)
	{
		return TOKENIZER_ENDOFINPUT;
	}

	if(isdigit(*ptr))
	{
		for(i = 0; i < MAX_NUMLEN; ++i)
		{
			if(!isdigit(ptr[i]))
			{
				if(i > 0)
				{
					nextptr = ptr + i;
					return TOKENIZER_NUMBER;
				}
				else
				{
					pbasic_error("too short number");
					return TOKENIZER_ERROR;
				}
			}
			if(!isdigit(ptr[i]))
			{
				pbasic_error("malformed number");
				return TOKENIZER_ERROR;
			}
		}
		pbasic_error("too long number");
		return TOKENIZER_ERROR;
	}
	else if(singlechar())
	{
		nextptr = ptr + 1;
		return singlechar();
	}
	else if(*ptr == '"')
	{
		nextptr = ptr;
		do {
			++nextptr;
		} while(*nextptr != '"');
		++nextptr;
		return TOKENIZER_STRING;
	}
	else
	{
		for(kt = keywords; kt->keyword != NULL; ++kt)
		{
			if(strncmp(ptr, kt->keyword, strlen(kt->keyword)) == 0)
			{
				nextptr = ptr + strlen(kt->keyword);
				return kt->token;
			}
		}
	}

	if(*ptr >= 'a' && *ptr <= 'z')
	{
		nextptr = ptr + 1;
		return TOKENIZER_VARIABLE;
	}

	return TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
void tokenizer_init(const char *prg)
{
	ptr = prg;
	current_token = get_next_token();
}
/*---------------------------------------------------------------------------*/
int tokenizer_token(void)
{
	return current_token;
}
/*---------------------------------------------------------------------------*/
void tokenizer_next(void)
{
	char buffer[MAX_STRINGLEN];

	if(tokenizer_finished())
	{
		return;
	}

	pbasic_debug("tokenizer_next(): %s", get_name_token(buffer, nextptr));
	ptr = nextptr;
	while(*ptr == ' ')
	{
		++ptr;
	}
	current_token = get_next_token();
	pbasic_debug("tokenizer_next(): %s token %d", get_name_token(buffer, ptr), current_token);
	return;
}
/*---------------------------------------------------------------------------*/
int tokenizer_num(void)
{
	return atoi(ptr);
}
/*---------------------------------------------------------------------------*/
void tokenizer_string(char *dest, int len)
{
	char *string_end;
	int string_len;

	if(tokenizer_token() != TOKENIZER_STRING)
	{
		return;
	}
	string_end = strchr(ptr + 1, '"');
	if(string_end == NULL)
	{
		return;
	}
	string_len = string_end - ptr - 1;
	if(len < string_len)
	{
		string_len = len;
	}
	memcpy(dest, ptr + 1, string_len);
	dest[string_len] = 0;
}
/*---------------------------------------------------------------------------*/
void tokenizer_error_print(void)
{
	char buffer[MAX_STRINGLEN];
	pbasic_error("invalid token %s", get_name_token(buffer, ptr));
}
/*---------------------------------------------------------------------------*/
int tokenizer_finished(void)
{
	return *ptr == 0 || current_token == TOKENIZER_ENDOFINPUT;
}
/*---------------------------------------------------------------------------*/
int tokenizer_variable_num(void)
{
	return *ptr - 'a';
}
/*---------------------------------------------------------------------------*/

#endif /* __TOKENIZER_C__ */
