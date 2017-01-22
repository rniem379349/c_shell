// Shell w C
// Autor: Robert Niemiec
// Data: 21.01.2017

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

char* read_cmnd();
char** read_args(char*);
int execute(char**);
int launch(char**);

void shell_loop()
{
  // pętla pobierająca komendę od użytkownika, przekształcająca ją na funkcję i argumenty, oraz wykonjująca odpowiednie funkcje.
  // funkcja ta rowniez zapamietuje historie komend wpisywanych przez uzytkownika.
  char *cmnd; // string z całą komendą
  char **args; // lista argumentów
  int status; // stan programu, od jego wartości zależy, czy pętla jest dalej wykonywana

  // inicjowanie historii polecen
  int i = 0, command;
  unsigned histbuf = 1024;
  char** hist = (char*)malloc(histbuf * sizeof(char*));
  if (!hist)
  {
    fprintf(stderr, "Blad alokacyjny\n");
    exit(0);
  }

  printf("Powłoka systemu Linux, w.1.0.\nSekcja pomocy - \"help\"\n\n");

  do {
    printf("> ");
    cmnd = read_cmnd(); // wczytujemy komendę

    // obsluga historii polecen
    hist[i] = malloc(histbuf * sizeof(char));
    strcpy(hist[i],cmnd);
    i++;

    if (i >= histbuf)
    {
      histbuf *= 2;
      hist = (char*)realloc(hist, histbuf * sizeof(char*));
      if (!hist)
      {
        fprintf(stderr, "Blad alokacyjny\n");
        exit(0);
      }
    }

    if (strcmp(cmnd,"history") == 0) // wyswietlenie historii polecen
    {
      command = 0;
      while (hist[command] != NULL)
      {
        printf("%d|\t%s\n", command, hist[command]);
        command++;
      }
      continue;
    }

    args = read_args(cmnd); // 'wyłuskanie' argumentów z komendy
    status = execute(args); // wykonanie odpowiedniej funkcji z argumentami

    // zwalnianie pamięci
    free(cmnd);
    free(args);
  } while (status);
  free(hist);
}


char* read_cmnd()
{
  int bufsize = 1024; // bufor dla poleceń
  int i = 0; // licznik petli przechodzacej po komendzie
  char *buffer = malloc(bufsize * sizeof(char));
  int c;

  if (!buffer)
  {
    fprintf(stderr, "Blad alokacyjny\n");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    c = getchar(); // czytamy pojedyncze znaki
    if (c == '\n') // jeśli napotkamy koniec linii ('enter'), to konczymy, zwracamy komendę
    {
      buffer[i] = '\0';
      return buffer;
    }
    else
    {
      buffer[i] = c; // dodajemy kolejne znaki do bufora
    }
    i++;

    // If we have exceeded the buffer, reallocate.
    if (i >= bufsize)
    {
      bufsize += 1024;
      buffer = realloc(buffer, bufsize);
      if (!buffer)
      {
        fprintf(stderr, "Blad alokacyjny\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}


char** read_args(char* cmnd)
{
  // printf("%s\n", cmnd); // debug
  int i = 0; // licznik przechodzacy po komendzie
  int cmnd_buffer = 1024; // bufor dla komendy
  char* delim = " "; // oddzielnik(i) slow w komendzie
  char** args = malloc(sizeof(char*) * cmnd_buffer); // komenda oraz argumenty, juz rozdzielone
  char* arg; // pojedyncze slowo

  if (!args)
  {
    fprintf(stderr, "Blad alokacyjny\n");
    exit(EXIT_FAILURE);
  }

  arg = strtok(cmnd, delim); // korzystajac z strtok() wyluskujemy pierwsze slowo

  while (arg != NULL) // powtarzamy proces dopoki mamy slowa w komendzie
  {
    args[i] = arg; // wszadzamy wyluskane slowo do znacznika w args
    i++;

    if (i >= cmnd_buffer) // zwiekszenie bufora na wypadek przekroczenia go
    {
      cmnd_buffer += cmnd_buffer;
      args = realloc(args, cmnd_buffer * sizeof(char*));

      if (!args)
      {
        fprintf(stderr, "Blad alokacyjny\n");
        exit(EXIT_FAILURE);
      }
    }
    arg = strtok(NULL, delim); // wyluskujemy kolejne slowa
  }
  args[i] = NULL; //jak juz nie ma slow, konczymy (odgradzamy) ilste argumentow nullem
  i = 0;
  // while (args[i] != NULL) // debug, printowanie argumentow
  // {
    // printf("%s\n", args[i]);
  //   i++;
  // }
  return args; // zwracamy liste slow
}


int launch(char** args)
{
  /*
  funkcja uruchamiajaca podana funkcje z podanymi argumentami
  **args - lista slow wchodzacych w sklad komendy, pierwsze slowo to funkcja, reszta to argumenty
  */
  int status;
  pid_t pid, wpid; // id procesu-dziecka i procesu-rodzica

  pid = fork(); // tworzymy forka dla procesu-dziecka (tworzy sie drugi proces)

  if (pid == 0) // tu wchodzi proces-dziecko, fork() zwraca 0 dla procesu-dziecka
  {
    if (execvp(args[0], args) == -1) // probujemy uruchomic funkcje, jesli cos poszlo nie tak zwracamy blad
    {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0) // jakis blad z pidem
  {
    perror("lsh");
  }
  else // tu wchodzi proces-rodzic
  {
    do
    {
      wpid = waitpid(pid, &status, WUNTRACED); // czekamy na ukonczenie procesu-dziecka
    } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // dopoki proces nie wyjdzie lub nie zostanie zabity
  }
  return 1;
}


//////// sekcja fukncji wbudowanych w shella (built-ins)

// wstepne deklaracje funkcji
int help(char** args);
int exit_shell(char** args);
int shell_pipe(char** args);
int shell_cd(char** args);

// lista built-inów
char* builtins[] =
{
  "help",
  "exit_shell",
  "shell_pipe",
  "shell_cd",
  "history"
};

int (*builtin_func[]) (char**) = // odnosniki do funkcji
{
  &help,
  &exit_shell,
  &shell_pipe,
  &shell_cd
};

int builtins_num() // ilosc funkcji wbudowanych
{
  return sizeof(builtins)/sizeof(builtins[0]);
}


int execute(char** args)
{
  int i;
  for (i = 0; i < builtins_num(); i++)
  {
    if (strcmp(args[0], builtins[i]) == 0)
    {
      return (builtin_func[i])(args);
    }
  }
  return launch(args);
}


int help(char** args)
{
  int i = 0;
  printf("Sekcja pomocy dla powloki.\nDostepne polecenia:\n\n");
  for (i; i < builtins_num(); i++)
  {
    printf("%s\n", builtins[i]);
  }
  printf("\n\n");
  return 1;
}


int shell_pipe(char** args)
{
  // Zwykla konkatenacja dwoch lub wiecej stringow
  int i = 2;
  int size = 0;
  unsigned pipe_buffer = 1024;
  if (args[1] == NULL || args[2] == NULL)
  {
    printf("shell_pipe przyjmuje co najmniej dwa argumenty\n");
    return 0;
  }

  char* result = malloc(pipe_buffer*sizeof(char*));
  size++;

  if (size >= pipe_buffer)
  {
    pipe_buffer = pipe_buffer * 2;
    result = realloc(result, pipe_buffer*sizeof(char*));
    if (!result)
    {
      printf("Blad alokacyjny\n");
      exit(EXIT_FAILURE);
    }
  }
  strcpy(result,args[1]);
  while (args[i] != NULL)
  {
    result = strcat(result, args[i]);
    i++;
    size++;

    if (size >= pipe_buffer)
    {
      pipe_buffer = pipe_buffer * 2;
      result = realloc(result, pipe_buffer*sizeof(char*));
      if (!result)
      {
        printf("Blad alokacyjny\n");
        exit(EXIT_FAILURE);
      }
    }
  }
  printf("%s\n", result);
  return 1;
}


int shell_cd(char** args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "Blad: wymagany folder docelowy\n");
  }
  if (chdir(args[1]) != 0)
  {
    fprintf(stderr,"shell_cd: Blad\n");
  }
  return 1;
}


int exit_shell(char** args)
{
  // po prostu wyjscie z petli (shella)
  printf("Do widzenia\n");
  return 0;
}

int main(int argc, char const *argv[])
{
  shell_loop();
  return 0;
}
