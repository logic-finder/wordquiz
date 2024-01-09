#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define MAX_WORDS 1000
#define LINE_MAX 1000

struct wmpair {
   char *word;
   char *mean;
};

void raise_err(char *, ...);
void clear_input_buffer(void);
int process_word_file(char *, struct wmpair *, int *);
int rand_range(int);
void shuffle(int *, int);
void make_quiz(int *, int, int, int, int *);
int pick_exclusive(int *, int, int);
int find_answer(int *, int, int);
void validate_answer(int *);

int main(int argc, char **argv) {
   /**************************
    * Program initialization *
    **************************/
   struct wmpair pairs[MAX_WORDS];
   int pairs_len;
   int process_result;
   
   if (argc != 2)
      raise_err("불충분한 인자 갯수; 사용법: wordquiz .dat");

   process_result = process_word_file(*(argv + 1), pairs, &pairs_len);
   if (process_result == -1)
      raise_err("단어장 파일이 비어있습니다.");
   else if (process_result == -2)
      raise_err("프로그램을 실행하기에 충분한 단어의 수가 아닙니다.");
   else if (process_result <= -3)
      raise_err("단어장 파일을 해석할 수 없습니다 (줄 %d); 종료", -process_result - 2);
   
   /**************
    * Title call *
    **************/
   printf("영단어를 외우자!\n\n");
   printf("만든사람: 코르\n레포지토리: https://github.com/logic-finder/wordquiz\n\n");

   /********************
    * Quiz preparation *
    ********************/
   int how_much;

   for (;;) {
      printf("몇 개의 단어를 테스트합니까? (최대 %d개) ", pairs_len);
      if (scanf("%d", &how_much))
         if (how_much > pairs_len)
            printf("입력된 수 %d는 최대 단어수 %d보다 클 수 없습니다.\n\n",
               how_much, pairs_len);
         else if (how_much <= 0)
            printf("테스트할 단어의 수는 0 이하일 수 없습니다.\n\n");
         else
            break;
      else
         printf("유효한 입력이 아닙니다.\n\n");
      clear_input_buffer();
   }
   clear_input_buffer();
   printf("\n");

   /* Make a quiz list */
   int quiz_list[pairs_len];

   for (int i = 0; i < pairs_len; i++)
      quiz_list[i] = i;
   srand(time(NULL));
   shuffle(quiz_list, pairs_len);

   /********************
    * Quiz in progress *
    ********************/
   int score = 0;
   int quiz_type;
   int delim[5];
   int answer;
   int response;
   int wrong_answer_count = 0;
   int wrong_answer[how_much];

   for (int i = 0; i < how_much; i++) {
      quiz_type = rand_range(2);

      if (quiz_type == 0) {
         printf("[문 %d] %s의 뜻으로 알맞은 것은?\n",
            i + 1, pairs[quiz_list[i]].word);

         make_quiz(delim, 5, quiz_list[i], pairs_len, &answer);
         
         for (int j = 0; j < 5; j++)
            printf("(%d) %s\n",
               j + 1, pairs[delim[j]].mean);
         
         validate_answer(&response);

         if (response - 1 == answer)
            score++;
         else
            wrong_answer[wrong_answer_count++] = quiz_list[i];
      }
      else {
         printf("[문 %d] %s의 뜻을 지닌 영단어로 알맞은 것은?\n",
            i + 1, pairs[quiz_list[i]].mean);

         make_quiz(delim, 5, quiz_list[i], pairs_len, &answer);

         for (int j = 0; j < 5; j++)
            printf("(%d) %s\n",
               j + 1, pairs[delim[j]].word);
         
         validate_answer(&response);

         if (response - 1 == answer)
            score++;
         else
            wrong_answer[wrong_answer_count++] = quiz_list[i];
      }
   }

   /***************
    * Quiz result *
    ***************/
   printf("퀴즈 결과: %d / %d\n", score, how_much);
   if (score == how_much)
      printf("완벽합니다!\n");
   else {
      printf("오답인 항목:\n");
      for (int i = 0; i < wrong_answer_count; i++)
         printf("%s | %s\n",
            pairs[wrong_answer[i]].word,
            pairs[wrong_answer[i]].mean);
      printf("약간의 분발이 필요하겠습니다.\n");
   }

   return 0;
}

/* 
 * raise_err: prints an format string at stderr.
 */
void raise_err(char *errmsg, ...) {
   va_list ap;
   
   va_start(ap, errmsg);
   vfprintf(stderr, errmsg, ap);
   va_end(ap);
   fprintf(stderr, "\n");
   exit(EXIT_FAILURE);
}

/*
 * clear_input_buffer: clears the stdin input buffer
 * so that the subsequent scanf calls work correctly.
 */
void clear_input_buffer(void) {
   int ch;
   /* Note: Always the left side is evaluated first. */
   while ((ch = fgetc(stdin)) != '\n' && ch != EOF);
}

/*
 * process_word_file: processes a given word file and
 * makes an array of type struct wmpair. For example:
 * [{ word: "otaku", mean: "십덕" },
 *  { word: "pizza", mean: "피자" }]
 */
int process_word_file(char *filename, struct wmpair *arr, int *arr_len) {
   /* Attempt to open file */
   FILE *word_file;

   if ((word_file = fopen(filename, "r")) == NULL)
      raise_err("단어장 파일을 열 수 없습니다; 종료");

   /* Check whether an empty file */
   int ch;

   ch = getc(word_file);
   if (feof(word_file)) {
      if (fclose(word_file))
         raise_err("파일 닫기의 실패: (Line %d)", __LINE__);
      return -1;  /* empty file */
   }
   if (ferror(word_file)) {
      if (fclose(word_file))
         raise_err("파일 닫기의 실패: (Line %d)", __LINE__);
      raise_err("파일로부터 문자 읽기의 실패: (Line %d)", __LINE__);
   }
   ungetc(ch, word_file);

   /* Read file */
   int count = 0;

   for (; count < MAX_WORDS; count++) {
      char line[LINE_MAX];
      char *s;
      char *word, *mean;
      
      s = fgets(line, LINE_MAX, word_file);
      if (s == NULL) {
         if (fclose(word_file))
            raise_err("파일 닫기의 실패: (Line %d)", __LINE__);
         raise_err("파일로부터 문자열 읽기의 실패: (Line %d)", __LINE__);
      }

      /* No need to check (s == NULL), since we've
         already checked the file is not empty. */
      s = strtok(line, "|");
      word = malloc(strlen(s) + 1);
      if (word == NULL) {
         if (fclose(word_file))
            raise_err("파일 닫기의 실패: (Line %d)", __LINE__);
         raise_err("메모리 동적 할당의 실패: (Line %d)", __LINE__);
      }
      strcpy(word, s);

      s = strtok(NULL, "\n");
      if (s == NULL)
         return -3 - count;   /* The line where this
                                 read error has occurred. */
      mean = malloc(strlen(s) + 1);
      if (mean == NULL) {
         if (fclose(word_file))
            raise_err("파일 닫기의 실패: (Line %d)", __LINE__);
         raise_err("메모리 동적 할당의 실패: (Line %d)", __LINE__);
      }
      strcpy(mean, s);

      arr[count].word = word;
      arr[count].mean = mean;

      if (feof(word_file))
         break;
   }
   *arr_len = count + 1;

   if (fclose(word_file))
      raise_err("파일 닫기의 실패: (Line %d)", __LINE__);

   return (count > 5) ? 0 : -2;  /* Not enough number of word */
}

/*
 * rand_range: returns a random integer in range of [0, max).
 */
int rand_range(int max) {
   return rand() / (RAND_MAX / max + 1);
}

/*
 * shuffle: shuffles the given array of type integer.
 */
void shuffle(int *arr, int len) {
   int i;
   int temp;
   int rv;
   
   for (i = 0; i < len; i++) {
      rv = rand_range(len);
      temp = arr[i];
      arr[i] = arr[rv];
      arr[rv] = temp;
   }
}

/*
 * make_quiz: makes a set of multiple choices for each question.
 */
void make_quiz(int *arr, int len, int initial, int total, int *answer) {
   arr[0] = initial;
   for (int j = 1; j < len; j++)
      arr[j] = pick_exclusive(arr, j, total);
   shuffle(arr, len);
   *answer = find_answer(arr, len, initial);
   if (*answer == -1)
      raise_err("퀴즈 출제중의 오류: (Line %d)", __LINE__);
}

/*
 * pick_exclusive: returns a random number which is
 * not in the given array (int *delim).
 */
int pick_exclusive(int *delim, int delim_len, int max) {
   int rv;
   int pass = 0;

   for (;;) {
      rv = rand_range(max);
      for (int i = 0; i < delim_len; i++)
         if (rv != delim[i])
            pass++;
      if (pass == delim_len)
         break;
      else
         pass = 0;
   }

   return rv;
}

/*
 * find_answer: returns the index where the third parameter
 * is in the given array (int *arr).
 */
int find_answer(int *arr, int len, int target) {
   int result = -1;

   for (int i = 0; i < len; i++) {
      if (arr[i] == target)
         result = i;
   }

   return result;
}

/*
 * validate_answer: checks whether the input
 * is correct.
 */
void validate_answer(int *response) {
   for (;;) {
      printf("\n답 입력: ");
      if (scanf("%d", response))
         if (1 <= *response && *response <= 5)
            break;
      printf("유효한 입력이 아닙니다.\n");
      clear_input_buffer();
   }
   clear_input_buffer();
   printf("\n");
}