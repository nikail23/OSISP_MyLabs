#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#define false 0
#define true 1

// 9.	Написать программу, находящую в заданном каталоге (первый аргумент командной строки) и всех его подкаталогах все файлы заданного расширения и создающий для каждого найденного файла жесткую ссылку в заданном каталоге. Расширение файла и каталог для жестких ссылок задаются в качестве второго и третьего аргументов командной строки.

char* ProgramName;

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

void ProceedErrors(char* Message, char* FilePath, char* LinkPath)
{
   fprintf(stderr, "%s: %s %s %s\n", basename(ProgramName), Message, FilePath, LinkPath);
}

bool ParametersCountCheck(int Count) 
{
	if (Count == 4) 
		return true;
	ProceedErrors("Error! Wrong console parameters count! Need 3 parameters.", "", "");
	return false;
}

void CreateHardLink(char* DirectoryPath, char* HardLinkDirectoryPath, char* ContentName) 
{
	char* LinkPath = Concat(Concat(HardLinkDirectoryPath, "/"), ContentName);
	char* FilePath = Concat(Concat(DirectoryPath, "/"), ContentName);
	if (link(FilePath, LinkPath) == -1)
	{	
		ProceedErrors(strerror(errno), FilePath, Concat(HardLinkDirectoryPath, "/"));
	}
	else
		printf("%s -> %s\n", Concat(Concat(DirectoryPath, "/"), ContentName), Concat(Concat(HardLinkDirectoryPath, "/"), ContentName));
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

void AnalyzeDirectoryContent(char* DirectoryPath, char* FilesExtension, char* HardLinkDirectoryPath)
{
	DIR* Directory;
	struct dirent* DirectoryContent;
	if (Directory = opendir(DirectoryPath))
	{
		errno = 0;
		DirectoryContent = readdir(Directory);
		if (errno)
			ProceedErrors(strerror(errno), DirectoryPath, Concat(HardLinkDirectoryPath, "/"));
		while (DirectoryContent != NULL)
		{
			char* ContentName = (*DirectoryContent).d_name;
			if (SelfOrParentDirectoryCheck(ContentName))
				if (NotCatalogCheck(DirectoryContent))
				{
					if (strstr(ContentName, FilesExtension) != 0)
						CreateHardLink(DirectoryPath, HardLinkDirectoryPath, ContentName);
				}
				else
				{
					char* NextDirectory = GetNextDirectoryPath(DirectoryPath, ContentName);
					AnalyzeDirectoryContent(NextDirectory, FilesExtension, HardLinkDirectoryPath);
				}
			errno = 0;
			DirectoryContent = readdir(Directory);
			if (errno)
				ProceedErrors(strerror(errno), DirectoryPath, Concat(HardLinkDirectoryPath, "/"));
		}
	}
	else
		ProceedErrors(strerror(errno), DirectoryPath, Concat(HardLinkDirectoryPath, "/"));
}

int main(int argc, char *argv[], char *envp[])
{
	if (ParametersCountCheck(argc)) 
	{
		ProgramName = argv[0];
		char* SourceDirectoryPath = argv[1];
		char* FilesExtension = argv[2];
		char* HardLinkDirectoryPath = argv[3];
		AnalyzeDirectoryContent(SourceDirectoryPath, FilesExtension, HardLinkDirectoryPath);
		return true;
	}
	return false;
}
