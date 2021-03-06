#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define RESET "\e[m"
#define RED "\e[1;31m"
#define GREEN "\e[1;32m"
#define YELLOW "\e[1;33m"
#define BLUE "\e[1;34m"
#define MAGENTA "\e[1;35m"
#define CYAN "\e[1;36m"
#define COLOURS 6

//#define DEBUG

typedef struct node
{
	struct node *next;
	char *word;
}node;

typedef struct tree
{
	node *argv;
	struct tree *left;
	struct tree *right;
	int Pipe;
}tree;

short eoflag = 0;
short Qflag = 0;
short Newlineflag = 0;
short Specflag = 0;
short Pipeflag = 0;
short Success = 0;
short Semicolon = 0;
short Badtree = 0;
short Appflag = 0;

int cur_pid = 0;

char*Colour[COLOURS] = {RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN};

char*dup_spec_symbols = ">|&";
char*one_spec_symbols = "<;";

int*pid_mas;

node*List;
tree*Root;

void delet_tree(tree*);

char*get_random_colour()
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	srand(time(NULL));
	int num = rand() % COLOURS;
	return Colour[num];
}

void print_pid()
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	int j;
	for(j = 1; j < pid_mas[0]; j++)
		printf("%d\t", pid_mas[j]);
	return;
}

int add_pid(int pid)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	int i = 1, size = pid_mas[0];
	while(pid_mas[i] != -1) i++;
	pid_mas[i] = pid;
	int result = i;
	i++;
	if(i == size)
	{
		size *= 2;
		pid_mas[0] = size;
		pid_mas = realloc(pid_mas, sizeof(int) * size);
		for(;i<size;i++) pid_mas[i] = -1;
	}
	return result;
}

void kill_children()
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	int j;
	for(j=1;j<pid_mas[0];j++)
		if(pid_mas[j] > 0)
		{
//			printf("Im gonna kill u, %d\n", pid_mas[j]);
			kill(pid_mas[j], SIGKILL);
//			pause();
		}
}

int pid_in_mas(int pid)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	int size = pid_mas[0];
	int i = 1;
	while(i < size)
	{
		if(pid_mas[i] == pid)
		{
			pid_mas[i] = -1;
			return i;
		}
		i++;
	}
	return -1;
}

char**list_to_mas(node*list)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	char**result = (char**)malloc(sizeof(char*));
	int i = 0;
	while(list)
	{
		result[i++] = list->word;
		result = realloc(result, sizeof(char*) * (i+1));
		list=list->next;
	}
	result[i] = NULL;
	return result;
}

node*insert(node*list, char*word)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(list==NULL)
	{
		list = (node*)malloc(sizeof(node));
		list->next = NULL;
		list->word = word;
		return list;
	}
	list->next = insert(list->next, word);
	return list;
}

void print_list(node*list)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(list)
	{
		printf("%s  ",list->word);
		print_list(list->next);
	}
}

void delet(node *list)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	node *tmp;
	while(list)
	{
		tmp=list->next;
		free(list->word);
		free(list);
		list=tmp;
	}
}

void delet_list(node *list)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(list)
	{
		delet_list(list->next);
		free(list);
	}
}

char*readword()
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	int size = 16, indx = 0;
	Specflag = 0;
	char*word = (char*)malloc(sizeof(char) * size);
	char c=0;
	while((c=getchar())!= EOF)
	{
		if(Qflag)
		{
			if(c=='"') Qflag = 0;
			else
			{
				indx++;
				if(indx == size)
				{
					size *= 2;
					word = realloc(word, sizeof(char) * size);
				}
				word[indx-1] = c;
			}
		}
		else if(isspace(c))
		{
			if(c=='\n') Newlineflag = 1;
			word[indx] = 0;
			return word;
		}
		else if(c=='"')
		{
			Qflag = 1;
		}
		else if(strchr(one_spec_symbols, c))
		{
			if(indx)
			{
				ungetc(c, stdin);
				word[indx]=0;
				return word;
			}
			else
			{
				word[0] = c;
				word[1] = 0;
				return word;
			}
		}
		else if(strchr(dup_spec_symbols, c))
		{
			if(Specflag)
			{
				if(word[0] == c)
				{
					word[1] = c;
					word[2] = 0;
				}
				else
				{
					ungetc(c, stdin);
					word[1] = 0;
				}
				return word;
			}
			else if(indx)
			{
				ungetc(c, stdin);
				word[indx] = 0;
				return word;
			}
			//First symbol in input is special
			Specflag = 1;
			word[indx++] = c;
		}
		else if(Specflag)
		{
			ungetc(c, stdin);
			word[1] = 0;
			return word;
		}
		else
		{
			indx++;
			if(indx==size)
			{
				size*=2;
				word = realloc(word, sizeof(char) * size);
				if(!word)
				{
					perror("Can't realloc");
					exit(1);
				}
			}
			word[indx-1] = c;
		}
	}
	word[indx] = 0;
	eoflag = 1;
	return word;
}

void check_exit(node*list)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(!strcmp(list->word, "exit"))
	{
		delet_tree(Root);
		delet(List);
		kill_children();
		free(pid_mas);
		printf("\n Exit \n");
		exit(0);
	}
}

int check_cd(char**argv)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(!strcmp(argv[0], "cd"))
	{
		if(argv[1]) chdir(argv[1]);
		else chdir(getenv("HOME"));
		return 0;
	}
	else return 1;
}

int do_list(node*list)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	check_exit(list);
	char**args = list_to_mas(list);
	if(check_cd(args))
	{
		free(args);
		return 0;
	}

	pid_t p;
	int status;
	if((p=fork())==0)
	{
		execvp(args[0], args);
		perror("Command error");
		exit(1);
	}
	wait(&status);
	free(args);
	return 0;
}

tree*create_node(char*word)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	tree*res=(tree*)malloc(sizeof(tree));
	res->right = NULL;
	res->left = NULL;
	res->argv = NULL;
	res->argv = insert(res->argv, word);
	res->Pipe = 0;
	return res;
}

void special_node(tree*tmp, char*word)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	while(tmp->right && !strcmp(tmp->right->argv->word, ";") ) tmp = tmp->right;
	tree*newnode = create_node(word);
	newnode->left = tmp->right;
	tmp->right = newnode;
}

tree*add_node(tree*res, char*word)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(!res)
	{
		res = create_node(word);
		return res;
	}
	tree*tmp = res;
	if(!strcmp(word, ";"))
	{
		if(Specflag || Pipeflag)
		{
			Badtree = 1;
			return res;
		}
		res = create_node(word);
		res->left = tmp;
		Specflag = 1;
		Semicolon = 1;
	}
	else if(!strcmp(word, "||") || !strcmp(word, "&&") || !strcmp(word, "&"))
	{
		if(Specflag || Pipeflag)
		{
			Badtree = 1;
			return res;
		}
		if(Semicolon) special_node(tmp, word);
		else
		{
			res = create_node(word);
			res->left = tmp;
		}
		Specflag = 1;
	}
	else if(!strcmp(word, "|"))
	{
		if(Specflag)
		{
			Badtree = 1;
			return res;
		}
		Specflag = 1;
		Pipeflag = 1;
	}
	else	//NEED FLAG OF IF THERE WERE SPECIAL SYMBOL BEFORE, IF SO THEN IT IS COMMAND OTHERWISE IT IS AN ARGUMENT
	{
		while(tmp->right) tmp=tmp->right;
		if(Specflag)
		{
			tmp->Pipe = Pipeflag;
			tmp->right = create_node(word);
			Specflag = 0;
			Pipeflag = 0;
		}
		else tmp->argv = insert(tmp->argv, word);
		return res;
	}
	return res;
}

tree*maketree(node*list)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	tree*res = NULL;
	while(list)
	{
		res = add_node(res, list->word);
		if(Badtree) return res;			//break
		list = list->next;
	}
	return res;
}

void print_tree(tree*T)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(T)
	{
		print_tree(T->left);
		print_list(T->argv);
		printf(" Pipe: %d", T->Pipe);
		printf("\n");
		print_tree(T->right);
	}
}

void delet_tree(tree*T)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(T)
	{
		delet_tree(T->left);
		delet_tree(T->right);
		delet_list(T->argv);
		free(T);
	}
}

int getfilein(tree*T)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	int fdin = 0;
	node*prev=T->argv;
	node*list=prev;
	while(list)
	{
		if(!strcmp(list->word, "<"))
		{
			if(!list->next)
			{
				return fdin;
			}
			fdin = open(list->next->word, O_RDONLY);
			if(fdin == -1)
			{
				perror("Input file err");
				return -1;
			}
			if(list == prev){T->argv = list->next; free(list); list = T->argv; T->argv = list->next; free(list); }
			else{prev->next = list->next; free(list); list=prev->next; prev->next = list->next; free(list); }
			return fdin;
		}
		prev=list;
		list = list->next;
	}
	return fdin;
}

int getfileout(tree*T)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	int fdout = 1;
	node*prev=T->argv;
	node*list=prev;
	while(list)
	{
		if(!strcmp(list->word, ">") || !strcmp(list->word, ">>") )
		{
			if(!list->next)
			{
				return fdout;
			}
			if(!strcmp(list->word, ">")) fdout = open(list->next->word, O_WRONLY | O_CREAT | O_TRUNC, 0664);
			else fdout = open(list->next->word, O_WRONLY | O_APPEND | O_CREAT, 0664);
			if(fdout == -1)
			{
				perror("Output file err");
				return -1;
			}
			if(list == prev){T->argv = list->next; free(list); list = T->argv; T->argv = list->next; free(list); }
			else{prev->next = list->next; free(list); list=prev->next; prev->next = list->next; free(list); }
			return fdout;
		}
		prev=list;
		list = list->next;
	}
	return fdout;
}

int catch_them_all(int (*get)(tree*), tree* T)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	int counter = 0;
	int fd, result;
	while((fd = get(T)) > 1)
	{
		result = fd;
		counter++;
	}
	if(fd == -1) return -1;
	if(!counter) result = fd;
	return result;
}

int run(tree*T, short pipes)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(!pipes)
	{
		check_exit(T->argv);
		int fd1 = catch_them_all(getfileout, T);
		int fd0 = catch_them_all(getfilein, T);
		if(fd0 == -1 || fd1 == -1) return 1;
		char**args = list_to_mas(T->argv);
		if(!check_cd(args))
		{
			if(fd1 != 1) close(fd1);
			if(fd0 != 0) close(fd0);
			free(args);
			return 0;
		}
		pid_t p;
		if((p=fork())==0)
		{
			if(!Appflag) signal(SIGINT,SIG_DFL);
			if(fd0 != 0){dup2(fd0, 0); close(fd0);}
			if(fd1 != 1){dup2(fd1, 1); close(fd1);}
			execvp(args[0], args);
			perror("comand exec err");
			delet_tree(Root);
			delet(List);
			free(args);
			free(pid_mas);
			exit(1);
		}
		free(args);
		if(fd1 != 1) close(fd1);
		if(fd0 != 0) close(fd0);
		int status = 0;
		if(!Appflag)
		{
			cur_pid = p;
			wait(&status);
			cur_pid = 0;
			if(WIFEXITED(status)) return WEXITSTATUS(status);
			else return -1;
		} else
		{
			int num = add_pid(p);
			printf("[%d] %d\n", num, p);
			return 0;
		}
	}
	else							// <<< P I P E >>>
	{
		pid_t p;
		int fd[2];
		int fread = dup(0);
		while(pipes--)
		{
			pipe(fd);
			if((p=fork())==0)
			{
				if(!Appflag) signal(SIGINT,SIG_DFL);
				dup2(fread, 0);
				dup2(fd[1], 1);
				close(fread);
				close(fd[0]);
				close(fd[1]);
				int fd1 = catch_them_all(getfileout, T);
				int fd0 = catch_them_all(getfilein, T);
				if(fd0 == -1 || fd1 == -1) exit(1);
				if(fd1 != 1){ dup2(fd1, 1); close(fd1);}
				if(fd0 != 0){ dup2(fd0, 0); close(fd0);}
				char**args = list_to_mas(T->argv);
				execvp(args[0], args);
				perror("pipe command exec err");
				free(args);
				delet_tree(Root);
				delet(List);
				free(pid_mas);
				exit(1);
			}
			//FATHER
			if(!Appflag) cur_pid = p;
			close(fd[1]);
			dup2(fd[0], fread);
			close(fd[0]);
			T = T->right;
		}
		if((p=fork())==0)
		{
			//LAST SON
			if(!Appflag) signal(SIGINT,SIG_DFL);
			dup2(fread, 0);
			close(fread);
			int fd1 = catch_them_all(getfileout, T);
			int fd0 = catch_them_all(getfilein, T);
			if(fd0 == -1 || fd1 == -1) exit(1);
			if(fd1 != 1){ dup2(fd1, 1); close(fd1);}
			if(fd0 != 0){ dup2(fd0, 0); close(fd0);}
			char**args=list_to_mas(T->argv);
			execvp(args[0], args);
			perror("last pipe command exec err");
			free(args);
			delet_tree(Root);
			delet(List);
			free(pid_mas);
			exit(1);
		}
		close(fread);
		int status = 0;
		if(!Appflag)
		{
			cur_pid = p;
			while(wait(&status) != -1);
			cur_pid = 0;
			if(WIFEXITED(status)) return WEXITSTATUS(status);
			else return -1;
		} else
		{
			int num = add_pid(p);
			printf("[%d] %d\n", num, p);
			return 0;
		}
	}
}

void do_tree(tree*T)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	if(T)
	{
		if(!strcmp(T->argv->word, ";"))
		{
			do_tree(T->left);
			do_tree(T->right);
		}
		else if(!strcmp(T->argv->word, "||"))
		{
			do_tree(T->left);
			if(!Success)
			do_tree(T->right);
		}
		else if(!strcmp(T->argv->word, "&&"))
		{
			do_tree(T->left);
			if(Success)
			do_tree(T->right);
		}
		else if(!strcmp(T->argv->word, "&"))
		{
			Appflag = 1;
			do_tree(T->left);;
		}
		else
		{
			tree*tmp=T;
			short pipes = 0;
			while(tmp)
			{
				pipes+=tmp->Pipe;
				tmp = tmp->right;
			}
			Success = !(run(T, pipes));
		}
	}
}

void MY_SIGCHLD(int SIG)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG func: %s\n", __func__);
#endif
	signal(SIGCHLD, MY_SIGCHLD);
	int status=0, pid, i;
	pid = waitpid(-1, &status, WNOHANG);
	if(pid > 0)
	{
		i = pid_in_mas(pid);
//		printf("Ind %d Pid %d\n", i, pid);
		if(i>=0)
		{
			if(WIFEXITED(status)) printf("[%d]+ Done\n", i);
			else if(WIFSIGNALED(status)) printf("[%d]+ Terminated\n", i);
			pid_mas[i] = -1;
		}
	}
}

int main(int argc, char **argv)
{
	char*w = NULL;
	pid_mas = NULL;
	pid_mas = (int*)malloc(sizeof(int) * 2);
	pid_mas[0] = 2;
	pid_mas[1] = -1;
	char*col = get_random_colour();
	signal(SIGCHLD, MY_SIGCHLD);
	signal(SIGINT, SIG_IGN);
	while(!eoflag)
	{
		printf("%s> %s", col, RESET);
		Newlineflag = 0;
		Specflag = 0;
		Pipeflag = 0;
		Semicolon = 0;
		Badtree = 0;
		Appflag = 0;
		List = NULL;
		Root = NULL;
		while(!eoflag && !Newlineflag)
		{
			w = readword();
			if(!strcmp(w, "&")) Appflag++;
			else if(Appflag) Appflag++;
			if(w[0]!=0) List = insert(List, w);
			else free(w);
		}
		Specflag = 0;
		if(Qflag)
		{
			fprintf(stderr, "Wrong numbers of quotes!\n");
			Qflag = 0;
			delet(List);
			continue;
		}
		if(Appflag > 1)
		{
			fprintf(stderr, "Wrong use of Appersant (&)\n");
			delet(List);
			continue;
		}
		if(!eoflag && List)
		{
			Root = maketree(List);
			if(Badtree)
			{
				fprintf(stderr, "Wrong sequence of spec symbols\n");
			}
			else
			{
				Appflag = 0;
				do_tree(Root);
			}
		}
		delet_tree(Root);
		delet(List);
	}
	kill_children();
	free(pid_mas);
	printf("\n%s Bye! %s\n", col, RESET);
	return 0;
}
