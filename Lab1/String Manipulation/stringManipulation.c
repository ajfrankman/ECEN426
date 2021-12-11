#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lowercase(char *myString) {
    for (size_t i = 0; i < strlen(myString); i++) {
        myString[i] = tolower(myString[i]);
    }
}

void uppercase(char *myString) {
    for (size_t i = 0; i < strlen(myString); i++) {
        myString[i] = toupper(myString[i]);
    }
}

void titlecase(char *myString) {
    for (size_t i = 0; i < strlen(myString); i++) {
        if (i == 0) { // first character should always be upper
            myString[0] = toupper(myString[0]);
        } else if (myString[i - 1] == ' ') {
            // If there was a space before it must be a new word. Make uppercase.
            myString[i] = toupper(myString[i]);
        } else {
            // If it isn't the first character and has not space before it, make it lowercase.
            myString[i] = tolower(myString[i]);
        }
    }
}

void reverse(char *myString) {
    int strLength = strlen(myString);
    for (int i = 0; i < strLength / 2; i++) {
        char temp = myString[i];                   // save current character.
        myString[i] = myString[strLength - i - 1]; // flip back character to front.
        myString[strLength - i - 1] = temp;        // flip saved front character to back.
    }
}