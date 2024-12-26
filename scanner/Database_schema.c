//
// Project: Database schema for SimpleSQL
//
// Prof. Joe Hummel
// Northwestern University
// CS 211, Winter 2023
//

#pragma once

//
// meta-data like table and column names have a max size 
// of 31, so this value is plenty safe for reading any of
// the meta-data, building filenames, etc.
//
#define DATABASE_MAX_ID_LENGTH 31

struct Database
{
  char*  name;
  int    numTables;
  struct TableMeta* tables;  // pointer to ARRAY of table meta-data
};

struct TableMeta
{
  char*  name;
  int    recordSize;
  int    numColumns;
  struct ColumnMeta* columns;  // pointer to ARRAY of column meta-data
};

struct ColumnMeta
{
  char* name;
  int   colType;    // int, real or string
  int   indexType;  // none, indexed, unique+indexed (aka Primary Key)
};

enum ColumnType
{
  COL_TYPE_INT = 1,
  COL_TYPE_REAL,
  COL_TYPE_STRING
};

enum IndexType
{
  COL_NON_INDEXED = 0,
  COL_INDEXED,         // indexed, could have duplicates
  COL_UNIQUE_INDEXED   // indexed, no duplicates
};


//
// functions:
//

//
// database_open
//
// Given a database name, tries to open the underlying database
// meta files and retrive the meta-data (i.e. database schema).
//
// Returns NULL if the database does not exist. Returns a pointer
// to a data structure if the database was successfully opened
// and the schema retrieved.
//
// NOTE: it is the callers responsibility to free the resources
// used by the data structure by calling database_close().
//
struct Database* database_open(char* database);

//
// database_close
//
// Frees the memory associated with the query; call this
// when you are done with the data structure.
//
void database_close(struct Database* db);

/*execute.c*/

//
// Project: Execution of queries for SimpleSQL
//
// SANDY BOCKARIE
// Northwestern University
// CS 211, Winter 2023
//
// Implementation of Files needed for this code.
#include <assert.h>
#include <ctype.h>
#include <stdbool.h> // true, false
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
//
// #include any other system <.h> files?
//

#include "analyzer.h"
#include "ast.h"
#include "database.h"
#include "execute.h"
#include "parser.h"
#include "resultset.h"
#include "scanner.h"
#include "tokenqueue.h"
#include "util.h"
//

//

//
// implementation of function(s), both private and public
//

void execute_query(struct Database *db, struct QUERY *query) {
  // checks if database exist
  if (db == NULL) {
    panic("database is NULL");
  }
  // checks if query exists
  if (query == NULL) {
    panic("query is NULL");
  }

  if (query->queryType != SELECT_QUERY) {
    printf("**Error: Cannot execute this query\n");
    return;
  }
  struct SELECT *select = query->q.select;

  struct TableMeta *table = NULL;
  // compare the name of the current table in the array with the stored string
  // in select-> table
  for (int i = 0; i < db->numTables; i++) {
    if (icmpStrings(db->tables[i].name, select->table) == 0) {
      table = &db->tables[i];
      break;
    }
  }
  assert(table != NULL);
  // creates a path to a file by combining the name of the database, a '/'
  // separator, the name of the table, and the ".data" extension. This path can
  // be used to access a file associated with the table in the database.
  char datapath[(2 * DATABASE_MAX_ID_LENGTH) + 10];
  strcpy(datapath, db->name);
  strcat(datapath, "/");
  strcat(datapath, table->name);
  strcat(datapath, ".data");

  FILE *file = fopen(datapath, "r");

  if (file == NULL) {
    printf("**Error: file '%s'is not found.", datapath);
    panic("stop execution");
  }
  struct ResultSet *result = resultset_create();
  // adding all the columns of the table to the result set with their respective
  // name, position, table name, and data type.
  for (int j = 0; j < table->numColumns; j++) {
    int columnNum = resultset_insertColumn(result, j + 1, table->name,
                                           table->columns[j].name, NO_FUNCTION,
                                           table->columns[j].colType);
  }
  // allocating memory dynamically to hold the contents of a table record. The
  // buffer is being allocated memory with the size of the record plus a few
  // bytes to hold additional data.
  int bufferSize = table->recordSize + 3;
  char *buffer = (char *)malloc(sizeof(char) * bufferSize);
  if (buffer == NULL) {
    panic("No memory");
  }
  // infinite loop starts here
  while (1) {
    fgets(buffer, bufferSize, file);
    if (feof(file)) {
      break;
    }

    bool k1 = false;
    bool k2 = false;

    char *cp = buffer;

    for(int j = 0; j < bufferSize; j++){
      if(!k1 && !k2){
        if(*cp == ' '){
          *(cp) = '\0';
        }
        else if(buffer[j]=='\''){
          k1 = true;
        }
        else if(buffer[j] == '"'){
          k2 = true;
        }
        }
        else if(k1){
          if(*cp == '\''){
            k1 = false;
          }
          }
          else{
            if(*cp == '"')
            {
              k2 = false;
            }
            }
          cp++;

        }
    
    int columnNumber = 1;
    cp = buffer;
    struct RSColumn *current = result->columns;
    int rowNumber = resultset_addRow(result);

    while(current != NULL){
      if(current->coltype == COL_TYPE_INT){
        resultset_putInt(result, rowNumber, columnNumber, atoi(cp));
        cp = cp + strlen(cp) + 1; // check this out
      }
      else if(current->coltype == COL_TYPE_REAL){
        resultset_putReal(result, rowNumber, columnNumber, atof(cp));
        cp = cp + strlen(cp) + 1;
      }
      else if(current->coltype == COL_TYPE_STRING){
        char *end = cp + strlen(cp) - 1;
        *end ='\0';
        cp++;
        resultset_putString(result, rowNumber, columnNumber, cp);
        cp = cp + strlen(cp) + 2;
      }
      columnNumber++;
      current = current->next;
    }
  }

  free(buffer);

  struct WHERE *where = select->where;
  if (where != NULL) {
    int columnNumber = resultset_findColumn(
        result, 1, where->expr->column->table, where->expr->column->name);
    char *value = where->expr->value;

    struct EXPR *express = where->expr;

    int oper = express->operator;

    char *columnName = express->column->name;

    int columnType = table->columns[columnNumber - 1].colType;

    for (int i = result->numRows; i > 0; i--) {
      if (columnType == 1) {
        int int_test = resultset_getInt(result, i, columnNumber);
        if (oper == 0) {
          if (int_test < atoi(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 1) {
          if (int_test <= atoi(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 2) {
          if (int_test > atoi(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 3) {
          if (int_test >= atoi(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 4) {
          if (int_test == atoi(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 5) {
          if (int_test != atoi(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else {
          resultset_deleteRow(result, i);
        }
      } else if (columnType == 2) {
        int double_test = resultset_getReal(result, i, columnNumber);
        if (oper == 0) {
          if (double_test < atof(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 1) {
          if (double_test <= atof(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 2) {
          if (double_test > atof(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 3) {
          if (double_test >= atof(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 4) {
          if (double_test == atof(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 5) {
          if (double_test != atof(value)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else {
          resultset_deleteRow(result, i);
        }
      } else if (columnType == 3) {
        char *string_test = resultset_getString(result, i, columnNumber);
        if (oper == 0) {
          if (strcmp(string_test, value) < 0) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 1) {
          if ((strcmp(string_test, value) < 0) ||
              (strcmp(string_test, value) == 0)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 2) {
          if (strcmp(string_test, value) > 0) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 3) {
          if ((strcmp(string_test, value) > 0) ||
              (strcmp(string_test, value) == 0)) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 4) {
          if (strcmp(string_test, value) == 0) {

          } else {
            resultset_deleteRow(result, i);
          }
        } else if (oper == 5) {
          if (strcmp(string_test, value) != 0) {

          } else {
            resultset_deleteRow(result, i);
          }
        }
        free(string_test);
      }
    }
  }

  // checking if all the columns of the table are present in the select
  // statement's columns. If any column is not present, it is removed from the
  // result set. The result set will only contain the columns that are present
  // in the select statement.
  for (int j = 0; j < table->numColumns; j++) {
    bool nan = false;
    struct ColumnMeta *columnMeta = table->columns;
    struct COLUMN *everyColumn = select->columns;
    struct COLUMN *iterate = everyColumn;

    while (iterate != NULL) {
      if (icmpStrings(iterate->name, columnMeta[j].name) == 0) {
        nan = true;
        break;
      }
      iterate = iterate->next;
    }

    if (nan == false) {
      int Q = resultset_findColumn(result, 1, table->name, columnMeta[j].name);
      resultset_deleteColumn(result, Q);
    }
  }

  int position = 1;

  struct COLUMN *iterate = select->columns;

  while (iterate != NULL) {
    int Q = resultset_findColumn(result, 1, iterate->table, iterate->name);
    resultset_moveColumn(result, Q, position);
    iterate = iterate->next;
    position++;
  }
  position = 1;
  iterate = select->columns;
  while (iterate != NULL) {
    if (iterate->function != NO_FUNCTION) {
      resultset_applyFunction(result, iterate->function, position);
    }
    iterate = iterate->next;
    position++;
  }
  // checking if a limit has been specified in the select statement and if so,
  // it is applying the limit to the result set.
  struct LIMIT *limit = select->limit;
  if (limit != NULL) {
    for (int i = result->numRows; i > 0; i--) {
      if (i > limit->N) {
        resultset_deleteRow(result, i);
      }
    }
  }
  // calling result_set_print() and resultset_destroy() and closing file
  resultset_print(result);
  resultset_destroy(result);
  fclose(file);
}
