#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "merc.h"
#include "libfile.h"
#ifndef S_SPLINT_S
#include <ctype.h>
#endif


/***************************************************************************
* IMPLEMENTATION
***************************************************************************/
long flag_convert(char letter)
{
	long bitsum = 0;
	int i;

	if ('A' <= letter && letter <= 'Z') {
		bitsum = 1;
		for (i = (int)letter; i > (int)'A'; i--)
			bitsum *= 2;
	} else if ('a' <= letter && letter <= 'z') {
		bitsum = 67108864; /* 2^26 */
		for (i = (int)letter; i > (int)'a'; i--)
			bitsum *= 2;
	}

	return bitsum;
}

/*
 * Read a letter from a file.
 */
char fread_letter(FILE *fp)
{
	char c;

	do
		c = (char)getc(fp);
	while (isspace((int)c));

	return c;
}


/*
 * Read a number from a file.
 */
long fread_long(FILE *fp)
{
	long number = 0;
	bool sign = false;
	char c;

	do
		c = (char)getc(fp);
	while (isspace((int)c));

	if (c == '+') {
		c = (char)getc(fp);
	} else if (c == '-') {
		sign = true;
		c = (char)getc(fp);
	}

	if (!isdigit((int)c)) {
		log_bug("Fread_long: bad format.", 0);
		raise(SIGABRT);
	}

	while (isdigit((int)c)) {
		number = number * 10 + (long)c - (long)'0';
		c = (char)getc(fp);
	}

	if (sign)
		number = 0 - number;

	if (c == '|')
		number += fread_long(fp);
	else if (c != ' ')
		ungetc(c, fp);

	return number;
}


/*
 * Read a number from a file.
 */
unsigned int fread_uint(FILE *fp)
{
	unsigned int number = 0;
	bool sign = false;
	char c;

	do
		c = (char)getc(fp);
	while (isspace((int)c));

	if (c == '+') {
		c = (char)getc(fp);
	} else if (c == '-') {
		sign = true;
		c = (char)getc(fp);
	}

	if (!isdigit((int)c)) {
		log_bug("Fread_uint: bad format.", 0);
		raise(SIGABRT);
	}

	while (isdigit((int)c)) {
		number = number * 10 + (unsigned int)c - (unsigned int)'0';
		c = (char)getc(fp);
	}

	if (sign)
		number = 65535u - number;

	if (c == '|')
		number += fread_uint(fp);
	else if (c != ' ')
		ungetc(c, fp);

	return number;
}



/*
 * Read a number from a file.
 */
int fread_number(FILE *fp)
{
	int number;
	bool sign;
	char c;

	do
		c = (char)getc(fp);
	while (isspace((int)c));

	number = 0;

	sign = false;
	if (c == '+') {
		c = (char)getc(fp);
	} else if (c == '-') {
		sign = true;
		c = (char)getc(fp);
	}

	if (!isdigit((int)c)) {
		log_bug("Fread_number: bad format.", 0);
        raise(SIGABRT);
	}

	while (isdigit((int)c)) {
		number = number * 10 + (int)c - (int)'0';
		c = (char)getc(fp);
	}

	if (sign)
		number = 0 - number;

	if (c == '|')
		number += fread_number(fp);
	else if (c != ' ')
		ungetc(c, fp);

	return number;
}


long fread_flag(FILE *fp)
{
	int number;
	char c;
	bool negative = false;

	do
		c = (char)getc(fp);
	while (isspace((int)c));

	if (c == '-') {
		negative = true;
		c = (char)getc(fp);
	}

	number = 0;

	if (!isdigit((int)c)) {
		while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
			number += flag_convert(c);
			c = (char)getc(fp);
		}
	}

	while (isdigit((int)c)) {
		number = number * 10 + (int)c - (int)'0';
		c = (char)getc(fp);
	}

	if (c == '|')
		number += fread_flag(fp);
	else if (c != ' ')
		ungetc(c, fp);

	if (negative)
		return -1 * number;

	return number;
}



/*
 * Read to end of line(for comments).
 */
void fread_to_eol(FILE *fp)
{
	char c;

	do
		c = (char)getc(fp);
	while (c != '\n' && c != '\r');

	do
		c = (char)getc(fp);
	while (c == '\n' || c == '\r');

	ungetc(c, fp);
	return;
}



/*
 * Read one word(into static buffer).
 */
char *fread_word(FILE *fp)
{
	static char word[MAX_INPUT_LENGTH];
	char *pword;
	char cEnd;

	do
		cEnd = (char)getc(fp);
	while (isspace((int)cEnd));

	if (cEnd == '\'' || cEnd == '"') {
		pword = word;
	} else {
		word[0] = cEnd;
		pword = word + 1;
		cEnd = ' ';
	}

	for (; pword < word + MAX_INPUT_LENGTH; pword++) {
		*pword = (char)getc(fp);
		if (cEnd == ' ' ? isspace((int)*pword) : *pword == cEnd) {
			if (cEnd == ' ')
				ungetc(*pword, fp);
			*pword = '\0';
			return word;
		}
	}

	log_bug("Fread_word: word too long.", 0);
	raise(SIGABRT);
    return 0;
}
