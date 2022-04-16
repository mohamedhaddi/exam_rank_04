/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mhaddi <mhaddi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/04/04 14:36:47 by mhaddi            #+#    #+#             */
/*   Updated: 2022/04/16 23:51:04 by mhaddi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int count_pipe_seperated_cmds(int i_argv, int argc, char **argv);
int get_command_size(int i_argv, int argc, char **argv);
void cd(int cmd_size, char **args);
void fatal();

int main(int argc, char **argv, char **env)
{
	if (argc == 1)
		return (0);

	int i_argv = 1; // index to loop through argv
	while (i_argv < argc) // i_argv will be incremented so that every iteration of this loop will be operating on the next semicolon-seperated command
	{
		/* do stuff for current semicolon-seperated commmad */

		int pipe_cmds_num = count_pipe_seperated_cmds(i_argv, argc, argv); // number of pipe-seperated commands in the current semicolon-seperated command
		pid_t pids[pipe_cmds_num]; // array of pids
		int i_pipe_cmds = 0; // index to loop through pipe-seperated commands
		int new_fds[2], old_fds[2]; // pipe fds

		while (i_pipe_cmds < pipe_cmds_num) // loop through all pipe-seperated commands
		{
			/* do stuff for current pipe-seperated commmad */

			// create args for execve
			int cmd_size = get_command_size(i_argv, argc, argv); // get the size of current command (program + number of args)
			char *args[cmd_size + 1]; // create args array, +1 for NULL
			int i = 0;
			while (i < cmd_size) // copy from argv to args
			{
				args[i] = argv[i_argv];
				i_argv++; // by the end of all iterations of this loop, i_argv will be the index of either: ";" or "|" or the end of argv (NULL)
				i++;
			}
			args[i] = NULL;

			/*
			 * BEGIN PIPING
			 */

			/*
			 * we're now ready to execve the current pipe-seperated command.
			 * but first we need to create a pipe and set its FDs.
			 * for that, I have used the following algorithm:
			 *
			 * ```
			 * for cmd in cmds
			 *     if there is a next cmd
			 *         pipe(new_fds)
			 *     fork
			 *     if child
			 *         if there is a previous cmd
			 *             dup2(old_fds[0], 0)
			 *             close(old_fds[0])
			 *             close(old_fds[1])
			 *         if there is a next cmd
			 *             close(new_fds[0])
			 *             dup2(new_fds[1], 1)
			 *             close(new_fds[1])
			 *         exec cmd || die
			 *     else
			 *         if there is a previous cmd
			 *             close(old_fds[0])
			 *             close(old_fds[1])
			 *         if there is a next cmd
			 *             old_fds = new_fds
			 * if there are multiple cmds
			 *     close(old_fds[0])
			 *     close(old_fds[1])
			 * ```
			 *
			 * this code avoids mangling the parent's stdin and stdout,
			 * it only manipulates them within the child, and never leaks any FDs.
			 *
			 * source: https://stackoverflow.com/a/917700/4273342
			 */

			if (i_pipe_cmds != pipe_cmds_num - 1) // if there is a next command
				if (pipe(new_fds) == -1) fatal(); // create pipe
			pid_t pid = fork(); // fork
			if (pid == -1) fatal();
			if (pid == 0) // child process
			{
				if (i_pipe_cmds != 0) // if there is a previous command
				{
					if (dup2(old_fds[0], 0) == -1) fatal();
					close(old_fds[0]);
					close(old_fds[1]);
				}
				if (i_pipe_cmds != pipe_cmds_num - 1) // if there is a next command
				{
					close(new_fds[0]);
					if (dup2(new_fds[1], 1) == -1) fatal();
					close(new_fds[1]);
				}
				
				// execute
				if (strcmp(args[0], "cd")) // if not cd, execve
				{
					if (execve(args[0], args, env) == -1)
					{
						write(2, "error: cannot execute ", 22);
						write(2, args[0], strlen(args[0]));
						write(2, "\n", 1);
						exit(1); // exit process with error
					}
				}
				else cd(cmd_size, args); // otherwise, chdir
			}
			else // parent process
			{
				pids[i_pipe_cmds] = pid; // save pid
				if (i_pipe_cmds != 0) // if there is a previous command
				{
					close(old_fds[0]);
					close(old_fds[1]);
				}
				if (i_pipe_cmds != pipe_cmds_num - 1) // if there is a next command
				{
					for (int i = 0; i < 2; i++)
						old_fds[i] = new_fds[i];
				}
			}

			/*
			 * END PIPING
			 */

			i_argv++; // escape the "|" or ";" argument (this is where we left off after copying from argv to args)
			i_pipe_cmds++; // continue to next pipe-seperated command
		}
		if (pipe_cmds_num > 1) // if there are multiple commands
		{
			close(old_fds[0]);
			close(old_fds[1]);
		}

		for (int i = 0; i < pipe_cmds_num; i++) // wait for all child processes to finish
			if (waitpid(pids[i], NULL, 0) == -1) fatal();

		// continue to next semicolon-seperated command
	}

	return (0);
}

void cd(int cmd_size, char **args)
{
	if (cmd_size != 2) // shouldn't be more or less than 2 args
	{
		write(2, "error: cd: bad arguments\n", 25);
		exit(1); // exit process with error
	}
	else
	{
		if (chdir(args[1]) == -1) // change directory
		{
			write(2, "error: cd: cannot change directory to ", 38);
			write(2, args[1], strlen(args[1]));
			write(2, "\n", 1);
			exit(1); // exit process with error
		}
		else
			exit(0); // exit process with success
	}
}

int count_pipe_seperated_cmds(int i_argv, int argc, char **argv)
{
	int i = i_argv;
	int count_pipe_cmds = 1;

	while (i < argc && strcmp(argv[i], ";"))
	{
		if (strcmp(argv[i], "|") == 0)
			count_pipe_cmds++;
		i++;
	}

	return (count_pipe_cmds);
}

int get_command_size(int i_argv, int argc, char **argv)
{
	int i = i_argv;
	int size = 0;
	while (i < argc && strcmp(argv[i], ";") && strcmp(argv[i], "|"))
	{
		size++;
		i++;
	}
	return (size);
}

void fatal()
{
	write(2, "error: fatal\n", 13);
	exit(1);
}