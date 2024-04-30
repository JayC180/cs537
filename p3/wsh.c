#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

char** history = NULL;
int hSize = 0;
int maxH = 5;
extern char** environ;

typedef struct {
    char* name;
    char* value;
} ShellVar;

ShellVar* sv = NULL;
int svn = 0;

void addH(char* cmd);
void exe(char* cmd);
void exeP(char* cmd);
void sh();
void shB(char *i);

void addV(char* name, char* value) {
	for(int i=0; i<svn; i++) {
		if(strcmp(sv[i].name, name)==0) {
			if(value[0]!='\0') {
				free(sv[i].value);
				sv[i].value = strdup(value);
			} else {
				free(sv[i].name);
				free(sv[i].value);
				if (svn > 1) {
					memmove(&sv[i], &sv[i + 1], (svn - i - 1) * sizeof(ShellVar));
				}
				svn--;
			}
			return;
		}
	}

	if (value[0] == '\0') {
		return;
	}

	ShellVar v;
	v.name = strdup(name);
	v.value = strdup(value);

	sv = realloc(sv, (svn+1) * sizeof(ShellVar));
	sv[svn] = v;
	svn++;
}

void subV(char* cmd) {
	for(int i=0; i<svn; i++) {
		char* vn = sv[i].name;
		char* vv = sv[i].value;
		int len = strlen(vn);
		int x = strcspn(cmd, vn);

		while (cmd[x] != '\0') {
			if (strncmp(&cmd[x], vn, len) == 0) {
				memcpy(&cmd[x], vv, strlen(vv));
				x += strlen(vv);
			} else {
				x++;
			}
		}
	}
}

void addH(char* cmd) {
	if(maxH==0) {
		return;
	}
	
	if(hSize==0) {
		history[0]=strdup(cmd);
		hSize++;
	} else if(strcmp(cmd, history[0])!=0) {
		int len = 0;
		if(hSize < maxH) {
			len = hSize;
		} else {
			len = maxH-1;
		}
		for(int i=len; i>0; i--) {
			history[i] = strdup(history[i-1]);
		}
		history[0] = strdup(cmd);
		if(hSize < maxH) {
			hSize++;
		} else {
			hSize = maxH;
		}
	}
}

void readH(char* i) {
	int n;
	if (sscanf(i, "history %d", &n) == 1 && n > 0 && n <= hSize) {
		char* cmd = history[n - 1];

		char* p = strchr(cmd, '|');
        if (p == NULL) {
            exe(cmd);
        } else {
            exeP(cmd);
        }
    } else {
        printf("Invalid history command\n");
        exit(-1);
    }


		
	/* exe(history[n-1]);
	} else {
		printf("read history f\n");
		exit(-1);
	}
	
	if (n<1 || n>hSize) {
		printf("read history size f\n");
		exit(-1);
	} else {
		exe(history[n-1]);
	}*/
}

void setH(int n) {
	if(n < 0) {
		printf("history size can't be negative\n");
		exit(-1);
	}

	//int x = maxH;
	
	maxH = n;

	if(hSize > maxH) {
		for(int i=hSize-1; i>=maxH; i--) {
			free(history[i]);
			history[i] = NULL;
		}
		hSize = maxH;
	}
	
	if (hSize < maxH) {
    	char** temp = realloc(history, maxH * sizeof(char*));
    	for(int i=hSize; i<maxH; i++) {
			temp[i] = NULL;
		}
		history = temp;
    }

	/*
	if(hSize < maxH && x > 0) {
		for(int i=0; i<hSize; i++) {
			printf("%d) %s\n", i+1, history[i]);
		}
	}
	*/
}


void cd(char* cmd) {
	char *d = strchr(cmd, ' ');
	if(d!=NULL) {
		while (*d==' ') {
			d++;
		}
		int len = strlen(d);
		if(len>0) {
			while(d[len-1]==' ' || d[len-1]=='\n') {
				d[len-1]='\0';
				len--;
			}
			if(chdir(d)!=0) {
				printf("cd f\n");
				exit(-1);
			}
		}
	} else {
		printf("cd arg f\n");
		exit(-1);
	}
}

void echo(char* cmd) {
	char *e = strchr(cmd, ' ');
	if(e!=NULL) {
		addH(cmd);
		while(*e==' ') {
			e++;
		}
		int len = strlen(e);
		while(e[len-1]==' ' || e[len-1]=='\n') {
			e[len-1]='\0';
			len--;
		}

		if (strncmp(e, "$", 1)==0) {
			char vn[64];
			strncpy(vn, e + 1, sizeof(vn));
			for (int i = 0; i < svn; i++) {
				if (strcmp(vn, sv[i].name) == 0) {
					printf("%s\n", sv[i].value);
					return;
				}
			}
			
		}

		printf("%s\n", e);
	} else {
		printf("echo arg f\n");
		exit(-1);
	}
}

void local(char* cmd) {
	char *v = strchr(cmd, ' ');
	if(v!=NULL) {
		v++;
		char* e = strchr(v, '=');
		if(e!=NULL) {
			*e = '\0';
			char* vn = v;
			char* vv = e + 1;

			addV(vn, vv);
		} else {
			printf("no e\n");
			exit(-1);
		}
	} else {
		printf("execvp: No such file or directory\n");
		return;
	}

}


void export(char* cmd) {
	char *v = strchr(cmd, ' ');
	if(v!=NULL) {
		v++;
		char* e = strchr(v, '=');
		
		if(e!=NULL) {
			*e = '\0';
			char* vn = v;
			char* vv = e+1;

			char cwd[1024];
			if((strcmp(vv, ".")==0) && (getcwd(cwd, sizeof(cwd)) != NULL)) {
				addV(vn, cwd);
				setenv(vn, cwd, 1);
			} else {
				addV(vn, vv);
				setenv(vn, vv, 1);
			}
		} else {
			printf("no e\n");
			exit(-1);
		}
	} else {
		printf("var NULL\n");
		exit(-1);
	}
}


void exe(char* cmd) {
	addH(cmd);

	pid_t pid = fork();
	
	if(pid == -1) {
		printf("fork f\n");
		exit(-1);
	} else if(pid == 0) {
		char* token = strtok(cmd, " ");
		if (strncmp(cmd, "bash -c", 7) == 0) {
            char* args[] = {"bash", "-c", cmd + 8, NULL};
            if (execvp("bash", args) == -1) {
                printf("execvp: No such file or directory\n");
                exit(-1);
            }
        } else {
			/*
			if (execvp(cmd, (char *const[]){cmd, NULL}) == -1) {
				printf("execvp: No such file or directory\n");
				exit(-1);
            }
            */
            
			char* args[1024];
			int i = 0;

			args[i++] = strdup(token);

			while ((token = strtok(NULL, " ")) != NULL) {
				args[i++] = strdup(token);
			}
			
			args[i] = NULL;

            if (execvp(args[0], args) == -1) {
                printf("execvp");
                exit(-1);
            }
            
        }
	} else {
		waitpid(pid, NULL, 0);
	}
}

void exeP(char* cmd) {	
    int n = 0;
    char* token = strtok(cmd, "|");
    char* commands[1024];

    while(token != NULL) {
        commands[n++] = strdup(token);
        token = strtok(NULL, "|");
    }

    int pipes[n-1][2];
    for (int i=0; i<n-1; i++) {
        if (pipe(pipes[i])==-1) {
            printf("pipe f\n");
            exit(-1);
        }
    }

    for (int i=0; i<n; i++) {
        pid_t pid = fork();

        if (pid==-1) {
            printf("fork f\n");
            exit(-1);
        } else if (pid==0) {
            if (i!=0) {
                if(dup2(pipes[i-1][0], STDIN_FILENO)==-1) {
                    printf("dup2 in f\n");
                    exit(-1);
                }
                close(pipes[i-1][0]);
            }

            if (i!=n-1) {
                if(dup2(pipes[i][1], STDOUT_FILENO)==-1) {
                    printf("dup2 out f\n");
                    exit(-1);
                }
                close(pipes[i][1]);
            }

            for (int j=0; j<n-1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            char* prog = strtok(commands[i], " ");
            char* args[1024];
            int iArg = 0;

            while (prog!=NULL) {
                args[iArg++] = strdup(prog);
                prog = strtok(NULL, " ");
            }

            args[iArg] = NULL;

            execvp(args[0], args);
            printf("execvp in pipe f\n");
            exit(-1);
        }
    }

    for(int i=0; i<n-1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i=0; i<n; i++) {
        wait(NULL);
    }

    for (int i=0; i<n; i++) {
        free(commands[i]);
    }
}


void sh() {
	char* i = NULL;
	size_t s = 0;
	
	while(1) {
		printf("wsh> ");

		ssize_t line = getline(&i, &s, stdin);
		if(line==-1) {
			free(i);
			// printf("\n");
			exit(0);
		}

		i[strcspn(i, "\n")] = 0;

		int isItV = 0;
		for(int j=0; j<svn; j++) {
			if(strcmp(i, sv[j].name)==0) {
				printf("%s\n", sv[j].value);
				isItV=1;
				continue;
			}
		}
		if(isItV) {
			continue;
		}

		/*
		if(getenv(i)!=NULL) {
			printf("this is an env");
			printf("%s\n", getenv(i));
			continue;
		}*/
		
		if(strchr(i, '|')!=NULL) {
			exeP(i);
			continue;
		}

		if(strcmp(i, "exit")==0) {
			free(i);
			exit(0);
		}

		if(strncmp(i, "cd", 2)==0) {
			cd(i);
			continue;
		}

		if(strncmp(i, "echo", 4)==0) {
			echo(i);
			continue;
		}

		if (strncmp(i, "local", 5) == 0) {
			local(i);
			continue;
		}

		if (strcmp(i, "vars")==0) {
			for(int i=0; i<svn; i++) {
				printf("%s=%s\n", sv[i].name, sv[i].value);
			}
			continue;
		}

		if (strncmp(i, "export", 6) == 0) {
			export(i);
			continue;
		}

		if(strncmp(i, "bash -c", 7)!=0) {
			if(strchr(i, '$')!=NULL) {
				subV(i);
				continue;
			}
		}
		
		if(strncmp(i, "history", 7)==0) {
			if(i[7]==' ') {
				if(i[8] == 's' && i[9] == 'e' && i[10] == 't' && i[11] == ' ') {
					if(i[12]  == '0') {
						setH(0);
					} else {
						int n = atoi(&i[12]);
						setH(n);
					}
				} else if (i[8] > '0'){
					int x = atoi(&i[8]);
					if (sscanf(i + 8, "%d", &x) == 1) {
						if (x > 0 && x <= hSize) {
							readH(i);
						}
					}
				}
			} else {
				if(hSize > 0) {
					for (int j=0; j<hSize; j++) {
						int a = j + 1;
				        printf("%d) %s\n", a, history[j]);
				    }	
			    }
			}
		} else {
			exe(i);
		}
	}
}

void shB(char *batch) {
	FILE *b = fopen(batch, "r");
	if(b==NULL) {
		printf("batch NULL\n");
		exit(-1);
	}

	/*
	fseek(b, 0, SEEK_END);
    if (ftell(b) == 0) {
        printf("batch empty\n");
        fclose(b);
        exit(-1);
    }
    rewind(b);

	char line[1024];
	*/

	char *i = NULL;
	size_t s = 0;

	while (getline(&i, &s, b) != -1) {
		i[strcspn(i, "\n")] = 0;

		int isItV = 0;
		for(int j=0; j<svn; j++) {
			if(strcmp(i, sv[j].name)==0) {
				printf("%s\n", sv[j].value);
				isItV=1;
				continue;
			}
		}
		if(isItV) {
			continue;
		}

		/*
		if(getenv(i)!=NULL) {
			printf("this is an env");
			printf("%s\n", getenv(i));
			continue;
		}*/
		
		if(strchr(i, '|')!=NULL) {
			exeP(i);
			continue;
		}

		if(strcmp(i, "exit")==0) {
			free(i);
			exit(0);
		}

		if(strncmp(i, "cd", 2)==0) {
			cd(i);
			continue;
		}

		if(strncmp(i, "echo", 4)==0) {
			echo(i);
			continue;
		}

		if (strncmp(i, "local", 5) == 0) {
			local(i);
			continue;
		}

		if (strcmp(i, "vars")==0) {
			for(int i=0; i<svn; i++) {
				printf("%s=%s\n", sv[i].name, sv[i].value);
			}
			continue;
		}

		if (strncmp(i, "export", 6) == 0) {
			export(i);
			continue;
		}

		if(strncmp(i, "bash -c", 7)!=0) {
			if(strchr(i, '$')!=NULL) {
				subV(i);
				continue;
			}
		}
		
		if(strncmp(i, "history", 7)==0) {
			if(i[7]==' ') {
				if(i[8] == 's' && i[9] == 'e' && i[10] == 't' && i[11] == ' ') {
					if(i[12]  == '0') {
						setH(0);
					} else {
						int n = atoi(&i[12]);
						setH(n);
					}
				} else if (i[8] > '0'){
					int x = atoi(&i[8]);
					if (sscanf(i + 8, "%d", &x) == 1) {
						if (x > 0 && x <= hSize) {
							readH(i);
						}
					}
				}
			} else {
				if(hSize > 0) {
					for (int j=0; j<hSize; j++) {
						int a = j + 1;
				        printf("%d) %s\n", a, history[j]);
				    }	
			    }
			}
		} else {
			exe(i);
		}
	}

	fclose(b);
	free(i);

	/*
	while (fgets(line, sizeof(line), b)) {
        line[strcspn(line, "\n")] = 0;
        char *pipePtr = strchr(line, '|');
   		if(pipePtr==NULL) {
   			exe(i);
   		} else {
   			exeP(i);
   		}
    }

    fclose(b);
    */
}

int main(int argc, char *argv[]) {
	if(argc > 2) {
		printf("too many args\n");
		exit(-1);
	}

	history = malloc(maxH * sizeof(char*));
	if (history == NULL) {
	    printf("history f\n");
	    exit(-1);
	}

	for (int i = 0; i < maxH; i++) {
	    history[i] = NULL;
	}
	
    if (argc == 2) {
    	shB(argv[1]);
    } else {
    	sh();
    }

    for (int i = 0; i < hSize; i++) {
        free(history[i]);
    }
    free(history);

	return 0;
}

