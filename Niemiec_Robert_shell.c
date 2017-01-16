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
  // pętla pobierająca komendę od użytkownika, przekształcająca ją na funkcję i argumenty, oraz wykonjująca odpowiednie funkcje
  char *cmnd; // string z całą komendą
  char **args; // lista argumentów
  int status; // stan programu, od jego wartości zależy, czy pętla jest dalej wykonywana


  do {
    printf("> ");
    cmnd = read_cmnd(); // wczytujemy komendę
    args = read_args(cmnd); // 'wyłuskanie' argumentów z komendy
    status = execute(args); // wykonanie odpowiedniej funkcji z argumentami

    // zwalnianie pamięci
    free(cmnd);
    free(args);
  } while (status);
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
    // Read a character
    c = getchar();
    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n')
    {
      buffer[i] = '\0';
      return buffer;
    }
    else
    {
      buffer[i] = c;
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
  while (args[i] != NULL)
  {
    printf("%s\n", args[i]);
    i++;
  }
  return args;
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
int print_history(char** args, char** hist, unsigned HIST_BUFFER);

// lista built-inów
char* builtins[] =
{
  "help",
  "exit_shell",
  "print_history"
};

int (*builtin_func[]) (char**) =
{
  &help,
  &exit_shell,
  &print_history
};

int builtins_num()
{
  return sizeof(builtins)/sizeof(builtins[0]);
}


int execute(char** args)
{
  int i;
  printf("Command: %s\n", args[0]);
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
  printf("Sekcja pomocy dla powloki.\nDostepne polecenia:\n");
  for (i; i < builtins_num(); i++)
  {
    printf("%s\n", builtins[i]);
  }
  return 1;
}


int print_history(char** args, char** hist, unsigned HIST_BUFFER)
{}

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
