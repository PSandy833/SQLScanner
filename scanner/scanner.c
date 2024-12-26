/*scanner.c*/

//
// Scanner for SimpleSQL programming language. The scanner
// reads the input stream and turns the characters into
// language Tokens, such as identifiers, keywords, and
// punctuation.
//
// Sandy Bockarie
// Northwestern University
// CS 211, Winter 2023
//
// Contributing author: Prof. Joe Hummel
//

#include <ctype.h>   // isspace, isdigit, isalpha
#include <stdbool.h> // true, false
#include <stdio.h>
#include <strings.h> // stricmp
#include <string.h>

#include "scanner.h"
#include "util.h"

//
// SimpleSQL keywords, in alphabetical order. Note that "static"
// means the array / variable is not accessible outside this file,
// which is intentional and good design.
//
static char *keywords[] = {"asc",  "avg",   "by",     "count",  "delete",
                           "desc", "from",  "inner",  "insert", "intersect",
                           "into", "join",  "like",   "limit",  "max",
                           "min",  "on",    "order",  "select", "set",
                           "sum",  "union", "update", "values", "where"};

static int numKeywords = sizeof(keywords) / sizeof(keywords[0]);

 static int isKeyword(char* value){
   for(int i=0; i < numKeywords; i++){
      if(strcasecmp(keywords[i], value) == 0)
      {
        return SQL_KEYW_ASC + i;
      }
   }
    return SQL_IDENTIFIER;
  }
// Returns true if real, return false if int
// Prefix can either be a digit, plus, or minus sign
static bool scanNum(FILE *input, char* value, char c, int *colNumber) {
  int count = 1;
  bool startsWithPeriod = false;
  
  value[count] = (char) c;
  count++;

  if (c != '.') {
    c = fgetc(input);
    while (isdigit(c)) {
      value[count] = (char) c;
      count++;
      c = fgetc(input);
    } 
  } else {
    startsWithPeriod = true;
  }

  if(c == '.') {
    if (!startsWithPeriod) {
      value[count] = '.';
      count++;
    }
    
    c = fgetc(input);
  
    while(isdigit(c))
    {
      value[count] = (char)c;
      count = count + 1;
      c = fgetc(input);
    }
    c = ungetc(c, input);
    value[count] = '\0';

    return true;
  } else {
    c = ungetc(c , input);
    value[count] = '\0';
    return false;
  }
}

//
// scanner_init
//
// Initializes line number, column number, and value before
// the start of the next input sequence.
//

void scanner_init(int *lineNumber, int *colNumber, char *value) {
  int count;
  if (lineNumber == NULL || colNumber == NULL || value == NULL)
    panic("one or more parameters are NULL (scanner_init)");

  *lineNumber = 1;
  *colNumber = 1;
  value[0] = '\0'; // empty string ""
}

//
// scanner_nextToken
//
// Returns the next token in the given input stream, advancing the line
// number and column number as appropriate. The token's string-based
// value is returned via the "value" parameter. For example, if the
// token returned is an integer literal, then the value returned is
// the actual literal in string form, e.g. "123". For an identifer,
// the value is the identifer itself, e.g. "ID" or "title". For a
// string literal, the value is the contents of the string literal
// without the quotes.
//
struct Token scanner_nextToken(FILE *input, int *lineNumber, int *colNumber,
                               char *value) 
{
  if (input == NULL)
    panic("input stream is NULL (scanner_nextToken)");
  if (lineNumber == NULL || colNumber == NULL || value == NULL)
    panic("one or more parameters are NULL (scanner_nextToken)");

  struct Token T;

  //
  // repeatedly input characters one by one until a token is found:
  //
  while (true) 
  {
    for(int i = 0; i < sizeof value ; ++i){
      value[i] = 0;
    }
    int c = fgetc(input);
    
    if (c == EOF) // no more input, return EOS:
    {
      T.id = SQL_EOS;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '$';
      value[1] = '\0';
      (*colNumber)++;
      return T;
    } 
  
    else if (isspace(c)) {
      if (c == '\n') {
        (*lineNumber)++;
        *colNumber = 1;
      } else {
        *colNumber += 1; 
      }
    }

    else if (c == '\n') {
      (*lineNumber)++;
      *colNumber = 1;
    }
       
    else if (c == '$') // this is also EOF / EOS
    {
      
      
      T.id = SQL_EOS;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
    else if (c == ';') 
    {
      
      T.id = SQL_SEMI_COLON;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
   
    else if (c == '>') // could be > or >=
    {
      //
      // peek ahead to the next char:
      //
      c = fgetc(input);
      
      if (c == '=') 
      {
        
        T.id = SQL_GTE;
        T.line = *lineNumber;
        T.col = *colNumber;

        value[0] = '>';
        value[1] = '=';
        value[2] = '\0';
        *colNumber = *colNumber + 2;
        return T;
      }

      //
      // if we get here, then next char did not form a token, so
      // we need to put char back to be processed on next call:
      //
      ungetc(c, input);
      

      T.id = SQL_GT;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '>';
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
    else if (c == '<') // could be > or >=
    {
      //
      // peek ahead to the next char:
      //
      c = fgetc(input);

      if (c == '=') 
      {
        
        T.id = SQL_LTE;
        T.line = *lineNumber;
        T.col = *colNumber;

        value[0] = '<';
        value[1] = '=';
        value[2] = '\0';
        *colNumber = *colNumber + 2;
        return T;
      } 
      else if (c == '>') 
      {
        T.id = SQL_NOT_EQUAL;
        T.line = *lineNumber;
        T.col = *colNumber;

        value[0] = '<';
        value[1] = '>';
        value[2] = '\0';

        *colNumber = *colNumber + 2;
        return T;
      }

      //
      // if we get here, then next char did not form a token, so
      // we need to put char back to be processed on next call:
      //
      ungetc(c, input);
      
      T.id = SQL_LT;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '<';
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
    else if (c == '(') // LEFT PAREN (
    {
      
      T.id = SQL_LEFT_PAREN;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
    else if (c == ')') // RIGHT PAREN )
    {
     
      T.id = SQL_RIGHT_PAREN;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
    else if (c == '*') // ASTERISK
    {
      
      T.id = SQL_ASTERISK;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
    else if (c == '#') // this is also EOF / EOS
    {
      
      T.id = SQL_HASH;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
    else if (c == '=') // this is also EOF / EOS
    {
      
      T.id = SQL_EQUAL;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '=';
      value[1] = '\0';
      
      (*colNumber)++;
      return T;
    } 
    else if (c == ',') // this is also EOF / EOS
    {
      
      T.id = SQL_COMMA;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = ',';
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
    else if (c == '.') // this is also EOF / EOS
    {
      
      T.id = SQL_DOT;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '.';
      value[1] = '\0';

      (*colNumber)++;
      return T;
    } 
    else if (c == '"') 
    {
      
      c = fgetc(input);
      int count = 0;


        while (c != '"' && c != '\n' && c != '$' && c != EOF)
        {
          value[count] = (char)c;
          count = count + 1;
          c =fgetc(input);
         
        }
        
        if ((c != '"') || (c == '$') || (c == EOF)) {
          printf("**WARNING: string literal @ (%d, %d) not terminated "
                "properly.\n", *lineNumber, (*colNumber));
        } 
       
        
    
          value[count] = '\0';  // should be looked at Pattan
        

        T.id = SQL_STR_LITERAL;
        T.line = *lineNumber;
        T.col = *colNumber;
        
        *colNumber = *colNumber + strlen(value)+2;
        return T;
       } 
    
   
    else if (c == '\'') 
    {
      
      c = fgetc(input);
      int count = 0;

        while (c != '\'' && c != '\n' && c != '$' && c != EOF)
        {
          value[count] = (char)c;
          count = count + 1;
          c =fgetc(input);
         
        }
        
        if ((c != '\'') || (c == '$') || (c == EOF)) {
          printf("**WARNING: string literal @ (%d, %d) not terminated "
                "properly.\n", *lineNumber, (*colNumber));
          // *lineNumber += 1;
          // *colNumber = 1;
        } 
       
          value[count + 1] = '\0';
        

        T.id = SQL_STR_LITERAL;
        T.line = *lineNumber;
        T.col = *colNumber;
        
        *colNumber = *colNumber + strlen(value)+2;
        return T;
       } 
      else if (isdigit(c))
    {
      int index  = 0;
      value[index] = (char)c;
      c = fgetc(input);

      bool isReal = false;
      
      if (isdigit(c) || c == '.') {
        isReal = scanNum(input, value, c, colNumber);
      } else {
        ungetc(c, input);
      }
      
      if (isReal) {
        T.id = SQL_REAL_LITERAL;
        T.line = *lineNumber;
        T.col = *colNumber;
        *colNumber = *colNumber + strlen(value);
        return T;
        
      } else {
        T.id =  SQL_INT_LITERAL;
        T.line = *lineNumber;
        T.col = *colNumber;
        *colNumber = *colNumber + strlen(value);
        return T;
      }
      
      
    } 
    else if (c == '-')
    { 
      c = fgetc(input);
      
      if((isspace(c)) || (c == EOF || c == '\n')) {
        ungetc(c, input);
        
        T.id = SQL_UNKNOWN;
        T.line = *lineNumber;
        T.col = *colNumber;
  
        value[0] = '-';
        value[1] = '\0';

        (*colNumber) += 1;
        // (*lineNumber)++;
        return T;
      }
      
      c = ungetc(c, input);
      
      int index = 0;
      value[0] = '-';
      c = fgetc(input);
      
      if(c == '-') // comment
      {
        while((c != '\n') && (c != EOF)){
          c = fgetc(input);
        }
        
        (*lineNumber)++;
         *colNumber = 1;
      } 
      else {
        //c = ungetc(c, input);
        bool isReal = scanNum(input, value, c, colNumber);
        if (isReal) {
          T.id = SQL_REAL_LITERAL;
          T.line = *lineNumber;
          T.col = *colNumber;

          *colNumber = *colNumber + strlen(value); 
          return T;
        } else {
          T.id =  SQL_INT_LITERAL;
          T.line = *lineNumber;
          T.col = *colNumber;
          *colNumber = *colNumber + strlen(value);
          return T;
        }
      }
     
    }
    else if (c == '+') {
      c = fgetc(input);
      if (isspace(c)) {
        ungetc(c, input);
        
        T.id = SQL_UNKNOWN;
        T.line = *lineNumber;
        T.col = *colNumber;
  
        value[0] = '+';
        value[1] = '\0';
        (*colNumber)++;
        return T;
      } else {
       // ungetc(c, input);
       // c = '+';
        value[0] = '+';
        bool isReal = scanNum(input, value, c, colNumber);
        if (isReal) {
          T.id = SQL_REAL_LITERAL;
          T.line = *lineNumber;
          T.col = *colNumber;

          *colNumber = *colNumber + strlen(value);
          return T;
        } else {
          value[0] = '+';
          T.id =  SQL_INT_LITERAL;
          T.line = *lineNumber;
          T.col = *colNumber;
          *colNumber = *colNumber + strlen(value);
          return T;
        }
      }
    }
    
    else if(isalnum(c)){
      
      int count = 0;
      value[count] = (char)c;
      count++;
      c = fgetc(input);
      
      // while(isalpha(c) || isdigit(c) || c == '_' ){
      //   value[count] = (char)c;
      //   count++;
      //   c = fgetc(input);
        
      // }

      while(isalnum(c) || c == '_' ){
        value[count] = (char)c;
        count++;
        c = fgetc(input);
        
      }
      c = ungetc(c, input);
      value[count] = '\0';

      T.id = isKeyword(value);
      T.line = *lineNumber;
      T.col = *colNumber;

      *colNumber += strlen(value);
      return T;
    }
    else 
    {
      //
      // if we get here, then char denotes an UNKNOWN token:
      //
      
      T.id = SQL_UNKNOWN;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      (*colNumber)++;
      return T;
    }

  } // while

  //
  // execution should never get here, return occurs
  // from within loop
  //
}
