#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>

#define maxsize 65535
#define false 0
#define true 1

char* ProgramName;
int MaxProcessesCount;
int ProcessesCount;
pid_t pid;

char* Concat(char *s1, char *s2)
{
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  char *result = malloc(len1 + len2 + 1);
  if (!result)
	{
    fprintf(stderr, "malloc() failed: insufficient memory!\n");
    return NULL;
  }
  memcpy(result, s1, len1);
  memcpy(result + len1, s2, len2 + 1);
  return result;
}

void ProceedErrors(char* Message, char* FilePath)
{
   fprintf(stderr, "%s: %s %s \n", basename(ProgramName), Message, FilePath);
}

bool ParametersCountCheck(int Count)
{
	if (Count == 4)
		return true;
	ProceedErrors("Error! Wrong console parameters count! Need 3 parameters.", "");
	return false;
}

bool SelfOrParentDirectoryCheck(char* ContentName)
{
	return ((strcmp(ContentName, ".") != 0) && (strcmp(ContentName, "..") != 0));
}

bool NotCatalogCheck(struct dirent* DirectoryContent)
{
	return ((*DirectoryContent).d_type != 4);
}

char* GetNextDirectoryPath(char* DirectoryPath, char* ContentName)
{
	return Concat(Concat(DirectoryPath, "/"), ContentName);
}

void Encrypt( char* SourceFileData, char* KeyFileData, char *CypherFileData )
{
	char* KeyFileBackupPointer = KeyFileData;
	while ( *SourceFileData )
	{
		if (!(*KeyFileData))
			KeyFileData = KeyFileBackupPointer;
		*CypherFileData++ = *SourceFileData++ ^ *KeyFileData++;
	}
}

void EncryptFile(char* SourceFilePath, char* KeyFilePath, char* CypherFilePath, struct stat SourceFileStatus)
{
  char SourceFileBuffer[maxsize];
  char KeyFileBuffer[maxsize];
  char CypherFileBuffer[maxsize];
  ssize_t FullReadSize, SourceFileReadInfo, KeyFileReadInfo;
  int SourceFileDescriptor, KeyFileDescriptor, CypherFileDescriptor;

  errno = 0;
  FullReadSize = 0;
  SourceFileDescriptor = open(SourceFilePath, O_RDONLY);
  if (errno)
  {
	  ProceedErrors(strerror(errno), SourceFilePath);\
	  return;
  }
  KeyFileDescriptor = open(KeyFilePath, O_RDONLY);
  if (errno)
  {
  	  ProceedErrors(strerror(errno), KeyFilePath);
  	  return;
    }
  CypherFileDescriptor = open(CypherFilePath, O_WRONLY | O_CREAT, SourceFileStatus.st_mode);
  if (errno)
  {
  	  ProceedErrors(strerror(errno), CypherFilePath);
      return;
  }

  while ((SourceFileReadInfo = read(SourceFileDescriptor, SourceFileBuffer, sizeof(SourceFileBuffer))) > 0)
  {
      if (errno)
      {
    	  ProceedErrors(strerror(errno), SourceFilePath);
    	  return;
      }
	  KeyFileReadInfo = read(KeyFileDescriptor, KeyFileBuffer, sizeof(KeyFileBuffer));
	  if (errno)
      {
	      ProceedErrors(strerror(errno), SourceFilePath);
	      return;
	  }
	  Encrypt(SourceFileBuffer, KeyFileBuffer, CypherFileBuffer);
	  char *out = CypherFileBuffer;
	  ssize_t WriteInfo;
	  do
	  {
		  FullReadSize += SourceFileReadInfo;
	      WriteInfo = write(CypherFileDescriptor, out, SourceFileReadInfo);
		  if (errno)
		  {
		      ProceedErrors(strerror(errno), SourceFilePath);
              return;
		  }
		  if (WriteInfo >= 0)
		  {
			  SourceFileReadInfo -= WriteInfo;
			  out += WriteInfo;
		  }
	  }
      while (SourceFileReadInfo > 0);
  }
  printf("%d %s %zu \n", getpid(), SourceFilePath, FullReadSize);
}

void ProcessesCountCheck()
{
	int status;
	while (ProcessesCount >= MaxProcessesCount)
	{
		printf("Count of processes is %d \n", ProcessesCount);
		wait(&status);
		ProcessesCount--;
	}
}

void AnalyzeDirectoryContent(char* DirectoryPath, char* KeyFilePath)
{
	DIR* Directory;
	struct dirent* DirectoryContent;
	if (Directory = opendir(DirectoryPath))
	{
		errno = 0;
		DirectoryContent = readdir(Directory);
		if (errno)
			ProceedErrors(strerror(errno), DirectoryPath);
		while (DirectoryContent != NULL)
		{
			char* ContentName = (*DirectoryContent).d_name;
			if (SelfOrParentDirectoryCheck(ContentName))
				if (NotCatalogCheck(DirectoryContent))
				{
					ProcessesCountCheck();
					char* SourceFilePath = Concat(Concat(DirectoryPath, "/"), ContentName);
					char* CypherFilePath = Concat(Concat(DirectoryPath, "/"), Concat("c", ContentName));
					ProcessesCount++;
					pid = fork();
					if (0 == pid)
  				    {
						struct stat SourceFileStatus;
						stat(SourceFilePath, &SourceFileStatus);
						EncryptFile(SourceFilePath, KeyFilePath, CypherFilePath, SourceFileStatus);
						exit(0);
					}
				}
				else
				{
					char* NextDirectory = GetNextDirectoryPath(DirectoryPath, ContentName);
					AnalyzeDirectoryContent(NextDirectory, KeyFilePath);
				}
			errno = 0;
			DirectoryContent = readdir(Directory);
			if (errno)
				ProceedErrors(strerror(errno), DirectoryPath);
		}
	}
	else
		ProceedErrors(strerror(errno), DirectoryPath);
}

int main(int argc, char *argv[])
{
  if (ParametersCountCheck(argc))
	{
		ProcessesCount = 0;
		ProgramName = argv[0];
		char* DirectoryPath = argv[1];
		char* KeyFilePath = argv[2];
	  MaxProcessesCount = atoi(argv[3]) - 1;
		AnalyzeDirectoryContent(DirectoryPath, KeyFilePath);
    ProcessesCountCheck();
		return true;
	}
	return false;
}

