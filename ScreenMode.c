#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

// List of outputs for xrandr

static char loc_TV[] = "HDMI-0";
static char loc_MonMain[] = "DP-0";
static char loc_MonSide[] = "DP-3";

static char hdmiIndex[] = "    ";
static char analogIndex[] = "    ";
//Finds the numerical index for a given sink name in pactl.

int pactlIndex(char sinkName[], char index[]) {
	int pip[2], pip2[2];
	pid_t pactlid;
	int status, status2;
	if (pipe(pip) == -1) {
		perror("pipe error \n");
		exit(1);	
	}
	pactlid = fork();
	if (pactlid == -1) {
		perror("fork error \n");
		exit(1);
	}
	if (pactlid == 0){//First child Process, writes the output of pactl to the pipe
		dup2(pip[1], STDOUT_FILENO);
		close(pip[1]);
		close(pip[0]);
		char* pactlarr[] = {"pactl", "list", "short", "sinks", NULL};
		execv("/bin/pactl", pactlarr);	
	}
	else {
		waitpid(pactlid, &status, 0);
		int buf2;
		close(pip[1]);
		dup2(pip[0], STDIN_FILENO);
		close(pip[0]);
		if (pipe(pip2) == -1) {
			perror("pipe 2 error \n");
			exit(1);
		}
		pid_t pactlid2 = fork();
		if (pactlid2 == -1) {
			perror("fork error \n");
			exit(1);
		}
		if (pactlid2 == 0) {//Second child process, uses grep to isolate the line with the desired sink name.
			dup2(pip2[1], STDOUT_FILENO);
			close(pip2[0]);
			close(pip2[1]);
			char* greparr[] = {"grep", sinkName, NULL};
			execv("/bin/grep", greparr);
			perror("grep failed");
		}
		else{//Parent process, isolates the index number and writes it to the external string.
			waitpid(pactlid2, &status2, 0);
		     	char buf;
			close(pip2[1]);
			int i = 0;
			while ((read(pip2[0], &buf, 1)> 0) && (buf != '\t') && buf != ' ') {
				index[i] = buf;
				i++;
			}
			close(pip2[0]);
			return 0;
		}
	}
};



	int TVMode() {
		pid_t pid=fork();
		if (pid == -1) {
			perror("fork error \n");
			exit(1);
		}
		if (pid == 0) {
			pid_t pid2=fork();
			if (pid2 == 0) {
				char* arr[] = {"xrandr", "--output", loc_MonSide, "--mode", "1920x1080", "--same-as", loc_TV, NULL};
				execv("/bin/xrandr", arr);
				printf("Process Complete\n");
			}
			pactlIndex("hdmi", hdmiIndex);
			char* audioarr[] = {"pactl", "set-default-sink", hdmiIndex, NULL};
			execv("/bin/pactl", audioarr);
		}
		printf("This is TV Mode\n");
		return 0;
	};
	int PCMode() {
		pid_t pidPC = fork();
		pactlIndex("analog", analogIndex);
		if (pidPC == 0) {
			char* audioarr[] = {"pactl", "set-default-sink", analogIndex, NULL};
                        execv("/bin/pactl", audioarr);
			printf("Process Failed.\n");
		}
		printf("This is PC Mode\n");
		return 0;
	};

int main(int argc, char* argv[]) {
	if (!strcmp(argv[1], "1")) {
		return TVMode();
	}	
	else if (!strcmp(argv[1], "2")){
		return PCMode();
	}
	else {
		printf("Error\n");
		if (argc > 1) {
			printf("%s\n", argv[1]);
		}
		return -1;
	}
}
