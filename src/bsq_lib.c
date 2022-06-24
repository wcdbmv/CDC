#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024


double bsq_number_input(const char* prompt) {
	printf("%s ", prompt);

	double value = 0.0;
	scanf("%lf", &value);

	while (getchar() != '\n') {
		// сожрат!
	}

	return value;
}

void bsq_number_print(double value) {
	printf("%lf\n", value);
}

char* bsq_text_input(const char* prompt) {
	printf("%s ", prompt);

	char buffer[BUFFER_SIZE] = {};
	fgets(buffer, BUFFER_SIZE - 1, stdin);
	size_t length = strlen(buffer);
	char* result = malloc(length);
	strncpy(result, buffer, length - 1);
	result[length - 1] = '\0';
	return result;
}

void bsq_text_print(const char* value) {
	printf("%s\n", value);
}

char* bsq_text_clone(const char* text) {
	char* result = malloc(1 + strlen(text));
	strcpy(result, text);
	return result;
}

char* bsq_text_conc(const char* lhs, const char* rhs) {
	size_t length = 1 + strlen(lhs) + strlen(rhs);
	char* result = malloc(length);
	strcpy(result, lhs);
	strcat(result, rhs);
	return result;
}

char* bsq_text_str(double d) {
	return NULL;
}

char* bsq_text_mid(const char* t, double b, double l) {
	return NULL;
}

bool bsq_text_eq(const char* lhs, const char* rhs) {
	return strcmp(lhs, rhs) == 0;
}

bool bsq_text_ne(const char* lhs, const char* rhs) {
	return strcmp(lhs, rhs) != 0;
}

bool bsq_text_gt(const char* lhs, const char* rhs) {
	return strcmp(lhs, rhs) > 0;
}

bool bsq_text_ge(const char* lhs, const char* rhs) {
	return strcmp(lhs, rhs) >= 0;
}

bool bsq_text_lt(const char* lhs, const char* rhs) {
	return strcmp(lhs, rhs) < 0;
}

bool bsq_text_le(const char* lhs, const char* rhs) {
	return strcmp(lhs, rhs) <= 0;
}
