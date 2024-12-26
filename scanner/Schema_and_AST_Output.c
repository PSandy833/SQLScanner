/*main.c*/

//
// Project 02: SCHEMA AND AST OUTPUT FOR SIMPLESQL

//  SANDY BOCKARIE
//  NORTHWESTERN UNIVERSITY
//  CS 211 WINTER 2023
//
//  CONTRIBUTING AUTHOR: PROF. JOE HUMMEL

// #include files
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "analyzer.h"
#include "ast.h"
#include "database.h"
#include "parser.h"
#include "scanner.h"
#include "util.h"

static char *var_col[] = {"int", "real", "string"};

static int var_colNum = sizeof(var_col) / sizeof(var_col[0]);

static char *var_index[] = {"non-indexed", "indexed", "unique indexed"};

static int var_indexNum = sizeof(var_index) / sizeof(var_index[0]);

static char *opers[] = {"<", "<=", ">", ">=", "=", "<>", "like"};

static int numOpers = sizeof(opers) / sizeof(opers[0]);

// print_schema function
// It prompts the user for the name of the database, then it prints the name of
// the database along with the details of each table such as name, record size,
// and details of each column such as name, type, and index type.
void print_schema(struct Database *db) {
  printf("**DATABASE SCHEMA**\n");
  // scanf("%s\n", db->name);
  printf("Database: ");
  printf("%s\n", db->name);
  struct TableMeta *tables = db->tables;

  for (int i = 0; i < db->numTables; i++) {
    printf("Table: %s\n", tables[i].name);
    printf("  Record size: %d\n", tables[i].recordSize);

    struct ColumnMeta *columns = tables[i].columns;
    for (int j = 0; j < tables[i].numColumns; j++) {
      printf("  Column: %s, %s, %s\n", columns[j].name,
             var_col[columns[j].colType - 1], var_index[columns[j].indexType]);
    }
  }
  printf("**END OF DATABASE SCHEMA**\n");
}
// It prints out the table and column name of the expression, the operator from
// the "opers" array and the value of the expression. If the operator is not
// 'LIKE' and the value is not a string, it prints the value as it is. If the
// value is a string it checks whether the string has single quotes and if it
// does it prints the string enclosed in double quotes, if not it prints it
// enclosed in single quotes. If none of the above conditions are met, it prints
// out "wrong expression under 'where'" and exits the program with exit code -1.

void exprPrint(struct EXPR *express) {
  int oper = express->operator;
  int lit = express->litType;
  printf("%s.%s %s ", express->column->table, express->column->name,
         opers[oper]);
  if (oper != EXPR_LIKE && lit != STRING_LITERAL) {
    printf("%s\n", express->value);
  } else if (lit == STRING_LITERAL) {
    if (strchr(express->value, '\'') != NULL) {
      printf("\"%s\"\n", express->value);
    } else {
      printf("'%s'\n", express->value);
    }
  } else {
    printf("wrong expression under 'where'\n");
    exit(-1);
  }
}

char *print_ast(struct QUERY *query) {
  printf("**QUERY AST**\n");
  struct SELECT *select = query->q.select;
  printf("Table: ");
  printf("%s\n", select->table);

  // Print columns
  struct COLUMN *col = select->columns;

  while (col != NULL) {
    printf("Select column: ");
    if (col->function == MIN_FUNCTION) {
      printf("MIN(%s.%s)\n", col->table, col->name);
    } else if (col->function == AVG_FUNCTION) {
      printf("AVG(%s.%s)\n", col->table, col->name);
    } else if (col->function == SUM_FUNCTION) {
      printf("SUM(%s.%s)\n", col->table, col->name);
    } else if (col->function == COUNT_FUNCTION) {
      printf("COUNT(%s.%s)\n", col->table, col->name);
    } else if (col->function == MAX_FUNCTION) {
      printf("MAX(%s.%s)\n", col->table, col->name);
    } else {
      printf("%s.%s\n", col->table, col->name);
    }
    col = col->next;
  }
  struct JOIN *join = select->join;
  printf("Join ");
  if (join == NULL) {
    printf("(NULL)\n");
  } else {
    printf("%s On %s.%s = %s.%s\n", join->table, join->left->table,
           join->left->name, join->right->table, join->right->name);
  }
  struct WHERE *where = select->where;
  printf("Where ");
  if (where == NULL) {
    printf("(NULL)\n");
  } else {
    exprPrint(where->expr);
  }
  struct ORDERBY *orderby = select->orderby;
  printf("Order By ");
  if (orderby == NULL) {
    printf("(NULL)\n");
  } else {
    if (orderby->column->function == MIN_FUNCTION) {
      printf("MIN(%s.%s) ", orderby->column->table, orderby->column->name);
    } else if (orderby->column->function == COUNT_FUNCTION) {
      printf("COUNT(%s.%s) ", orderby->column->table, orderby->column->name);
    } else if (orderby->column->function == SUM_FUNCTION) {
      printf("SUM(%s.%s) ", orderby->column->table, orderby->column->name);
    } else if (orderby->column->function == AVG_FUNCTION) {
      printf("AVG(%s.%s) ", orderby->column->table, orderby->column->name);
    } else if (orderby->column->function == MAX_FUNCTION) {
      printf("MAX(%s.%s) ", orderby->column->table, orderby->column->name);
    } else {
      printf("%s.%s ", orderby->column->table, orderby->column->name);
    }
    if (orderby->ascending) {
      printf("ASC\n");
    } else {
      printf("DESC\n");
    }
  }
  struct LIMIT *limit = select->limit;
  printf("Limit ");
  if (limit == NULL) {
    printf("(NULL)\n");
  } else {
    printf("%d\n", limit->N);
  }
  struct INTO *into = select->into;
  printf("Into ");
  if (into == NULL) {
    printf("(NULL)\n");
  } else {
    printf("%s\n", into->table);
  }
  printf("**END OF QUERY AST**\n");
  return select->table;
}

void execute_query(FILE *input, int rSize) {
  int acc = 0;
  int Size = rSize + 3;
  char *buff = (char *)malloc(sizeof(char) * Size);
  if (buff == NULL)
    panic("Memory Out\n");
  while (!feof(input) && acc < 5) {
    fgets(buff, Size, input);
    printf("%s", buff);
    acc += 1;
  }
}

// int main()
int main() {
  char dbs[DATABASE_MAX_ID_LENGTH + 1];
  printf("database? ");
  scanf("%s", dbs);

  struct Database *db = database_open(dbs);

  if (db == NULL) {
    printf("**Error: unable to open database '%s'\n", dbs);
    exit(-1);
  }

  print_schema(db);

  parser_init();
  while (1) {
    printf("query? ");
    struct TokenQueue *tokens = parser_parse(stdin);
    if (tokens == NULL) {
      if (parser_eof()) {
        break;
      } else {
        continue;
      }
    }
    struct QUERY *query = analyzer_build(db, tokens);
    if (query != NULL) {
      char *astTable = print_ast(query);

      char fname[DATABASE_MAX_ID_LENGTH + 1];
      strcpy(fname, db->name);
      strcat(fname, "/");

      struct TableMeta *tables = db->tables;
      int j = -1;
      for (int i = 0; i < db->numTables; i++) {
        if (strcasecmp(tables[i].name, astTable) == 0) {
          j = i;
          break;
        }
      }
      if (j == -1) {
        printf("**Error:wrong mismatch with table names\n");
        exit(-1);
      }
      strcat(fname, tables[j].name);
      strcat(fname, ".data");
      FILE *input = fopen(fname, "r");
      if (input == NULL) {
        printf("**Error: unable to open file '%s'", fname);
      }
      execute_query(input, tables[j].recordSize);
    }
  }
  database_close(db);
  return 0;
}


/*database.h*/
