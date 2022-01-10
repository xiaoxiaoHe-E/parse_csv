#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 *	Represents the end-of-line terminator type of the input
 */
typedef enum EolType { EOL_UNKNOWN, EOL_CR, EOL_LF, EOL_CRLF } EolType;

/**
 * @brief Get all eol in this buffer, and save the index
 * (relative index in this buffer) of eol in eol_idx
 * when eol is cr or lf
 *
 * @param eol_idx a list to save the index of eol in this buffer
 * @param len the length of this buffer
 * @param fstr buffer of data
 * @param qc quote char
 * @param xc escape char
 * @param eol end of line char
 *
 */
void get_eol_cr_or_lf(int *eol_idx, int len, char *fstr, int qc, int xc,
                      int eol) {
  int in_quote = 0;
  int last_was_esc = 0;
  int ch = 0;
  int i = 0;
  int res_len = 0;

  for (i = 0; i < len && fstr[i] != '\0'; i++) {
    ch = fstr[i];
    if (in_quote) {
      if (!last_was_esc) {
        if (ch == qc)
          in_quote = 0;
        else if (ch == xc)
          last_was_esc = 1;
      } else
        last_was_esc = 0;
    } else if (ch == eol) {
      eol_idx[res_len] = i;
      res_len++;
    } else if (ch == qc) {
      in_quote = 1;
    }
  }
}

/**
 * @brief Get all eol in this buffer, and save the index
 * (relative index in this buffer) of eol(last char) in eol_idx
 * when eol is crlf
 *
 * @param eol_idx a list to save the relative index of eol in this buffer
 * @param len the length of this buffer
 * @param fstr buffer of data
 * @param qc quote char
 * @param xc escape char
 *
 */
void get_eol_crlf(int *eol_idx, int len, char *fstr, int qc, int xc) {
  int in_quote = 0;
  int last_was_esc = 0;
  int ch = 0;
  int lastch = 0;
  int i = 0;
  int res_len = 0;

  for (i = 0; i < len && fstr[i] != '\0'; i++) {
    lastch = ch;
    ch = fstr[i];
    if (in_quote) {
      if (!last_was_esc) {
        if (ch == qc)
          in_quote = 0;
        else if (ch == xc)
          last_was_esc = 1;
      } else
        last_was_esc = 0;
    } else if (ch == '\n' && lastch == '\r') {
      eol_idx[res_len] = i;
      res_len++;
    } else if (ch == qc) {
      in_quote = 1;
    }
  }
}


/**
 * @brief print the index of every chunk, for parrent process to get the index form 
 * pipe stdout
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[]) {
  int i = 0;
  char *fpath;
  char qc = '\'';
  char xc = '\\';
  EolType eol_type = EOL_LF;
  int bs = 32768;
  int cs = 64 * 1024;

  while (i < argc) {
    if (strcmp(argv[i], "-f") == 0) {
      fpath = (char *)calloc(strlen(argv[i + 1]), sizeof(char));
      strcpy(fpath, argv[i + 1]);
      i += 2;
    } else if (strcmp(argv[i], "-q") == 0) {  //quote char
      qc = argv[i + 1][0];
      i += 2;
    } else if (strcmp(argv[i], "-x") == 0) {  //escape char
      xc = argv[i + 1][0];
      i += 2;
    } else if (strcmp(argv[i], "-eol") == 0) {  //eol
      if (strcmp(argv[i + 1], "CR") == 0) {
        eol_type = EOL_CR;
      } else if (strcmp(argv[i + 1], "LF") == 0) {
        eol_type = EOL_LF;
      } else if (strcmp(argv[i + 1], "CRLF") == 0) {
        eol_type = EOL_CRLF;
      } else {
        fprintf(stderr, "eol shoud be one of CR, LF, CRLF\n");
        exit(-1);
      }
      i += 2;
    } else if (strcmp(argv[i], "-bs") == 0) {  //read buffer size
      bs = atoi(argv[i + 1]);
      i += 2;
    } else if (strcmp(argv[i], "-cs") == 0) {  //chunk size
      cs = atoi(argv[i + 1]);
      i += 2;
    } else
      i++;
  }

  FILE *fp;
  if ((fp = fopen(fpath, "r")) == NULL) {
    fprintf(stderr, "cannot op file %s\n", fpath);
    exit(-1);
  }
  fseek(fp, 0, SEEK_END);
  int filesize = ftell(fp);
  rewind(fp);

  char *buffer;
  buffer = (char *)calloc(bs, sizeof(char));
  int cur_buffer = 0;
  int *eol_idx;
  eol_idx = (int *)calloc(bs, sizeof(int));
  int chunk_st = 0;  // start index(absolute index in the file) of current chunk
  int cur_chunk = 0;  // the size of current chunk
  int numread = 0;
  int read_time = 0;
  while (1) {
    if (feof(fp)) break;
    numread = fread(buffer + cur_buffer, sizeof(char), bs - cur_buffer, fp);
    read_time++;
    memset(eol_idx, -1, bs * sizeof(int));

    int eol_cnt = -1;
    switch (eol_type) {
      case EOL_CR:
        get_eol_cr_or_lf(eol_idx, numread + cur_buffer, buffer, qc, xc, '\r');
        break;
      case EOL_LF:
        get_eol_cr_or_lf(eol_idx, numread + cur_buffer, buffer, qc, xc, '\n');
        break;
      case EOL_CRLF:
        get_eol_crlf(eol_idx, numread + cur_buffer, buffer, qc, xc);
        break;
      default:
        get_eol_cr_or_lf(eol_idx, numread + cur_buffer, buffer, qc, xc, '\n');
        break;
    }

    // do not found a eol in this buffer, so the element in eol_idx is -1
    if (eol_idx[0] == -1) {
      fprintf(stderr,
              "line too long in file %s\nplease set buffer size larger\n",
              fpath);
      exit(-1);
    }

    for (int i = 0; i < bs && eol_idx[i] >= 0; i++) {
      eol_cnt++;
      int next_line = i == 0 ? eol_idx[0] + 1 : eol_idx[i] - eol_idx[i - 1];
      // maybe chunk_size < line_len
      if (next_line > cs) {
        fprintf(stderr,
                "line too long for chunk size\nplease set chunk size larger\n");
        exit(-1);
      }
      // current chunk size add net line size is larger than the chunk size
      // we see current chunk as a full chunk
      // next line belongs to the next chunk
      if (cur_chunk + next_line > cs) {
        chunk_st += cur_chunk;  // the end of this chunk
        printf("%d\n", chunk_st);
        cur_chunk = next_line;
      } else  // this chunk is not full, add next line to current chunk
      {
        cur_chunk += next_line;
      }
    }
    // move the data after the last eol to the start of the buffer
    // make sure a line would not be splitted to different buffers
    // and every start of a buffer is a start of a line
    memmove(buffer, buffer + eol_idx[eol_cnt] + 1, bs - eol_idx[eol_cnt] - 1);
    cur_buffer = bs - eol_idx[eol_cnt] - 1;
  }
  if (filesize - chunk_st > cs) {  // some data last after the last chunk
    printf("%d\n", cur_chunk + chunk_st);
    printf("%d\n", filesize);
  } else {  // may have a last line without a eol, is not included in cur_chunk
    printf("%d\n", filesize);
  }

  fclose(fp);
  return 1;
}
