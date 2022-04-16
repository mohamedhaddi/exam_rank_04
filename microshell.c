/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mhaddi <mhaddi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/04/04 14:36:47 by mhaddi            #+#    #+#             */
/*   Updated: 2022/04/16 18:11:50 by mhaddi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

void fatal() // to call whenever a system call (other than execve and chdir) fails
{
	write(2, "error: fatal\n", 13);
	exit(1);
}

int count_semicolon_seperated_cmds(int argc, char **argv)
{
	int i = 1;
	int count_semicolon_cmds = 1;
	while (i < argc)
	{
		if (strcmp(argv[i], ";") == 0)
			count_semicolon_cmds++;
		i++;
	}
	return (count_semicolon_cmds);
}

int count_pipe_seperated_cmds(int argc, char **argv)
{
	static int i = 1;
	int count_pipe_cmds = 1;

	while (i < argc)
	{
		if (strcmp(argv[i], ";") == 0)
		{
			i++;
			break;
		}
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
	while (i < argc)
	{
		if (strcmp(argv[i], ";") == 0 || strcmp(argv[i], "|") == 0)
			break;
		size++;
		i++;
	}
	return (size);
}

int main(int argc, char **argv, char **env)
{
	if (argc == 1)
		return (0);

	// count how many commnads are seperated by ; and create an array of ints
	// to store the numbers of commands seperated by | in each command
	int semicolon_cmds_num = count_semicolon_seperated_cmds(argc, argv);
	int semicolon_cmds[semicolon_cmds_num];

	// for each command seperated by ;, count how many commands are seperated by |
	int i = 0;
	while (i < semicolon_cmds_num)
	{
		semicolon_cmds[i] = count_pipe_seperated_cmds(argc, argv);
		i++;
	}

	int i_argv = 1; // argv index
	int i_semicolon_cmds = 0; // semicolon_cmds index

	while (i_semicolon_cmds < semicolon_cmds_num) // loop through the commands seperated by ;
	{
		/* do stuff for current semicolon-seperated commmad */

		int i_pipe_cmds = 0; // pipe_cmds index
		int pipe_cmds_num = semicolon_cmds[i_semicolon_cmds]; // number of pipe-seperated cmds in current semicolon-seperated cmd
		int new_fds[2], old_fds[2]; // pipe fds
		pid_t pids[pipe_cmds_num]; // array of pids

		while (i_pipe_cmds < pipe_cmds_num) // loop through the commands seperated by |
		{
			/* do stuff for current pipe-seperated commmad */

			// create args for execve
			int cmd_size = get_command_size(i_argv, argc, argv); // get the size of current command (program + number of args)
			char *args[cmd_size + 1]; // create args array, +1 for NULL
			int i = 0;
			while (i < cmd_size) // copy from argv to args
			{
				args[i] = argv[i_argv];
				i_argv++;
				i++;
			}
			args[i] = NULL;

			// pipes
			if (i_pipe_cmds != pipe_cmds_num - 1) // if there is a next command
				pipe(new_fds); // TODO: if failed, call fatal()
			pid_t pid = fork(); // TODO: if failed, call fatal()
			if (pid == 0)
			{
				if (i_pipe_cmds != 0) // if there is a previous command
				{
					dup2(old_fds[0], 0); // TODO: if failed, call fatal()
					close(old_fds[0]);
					close(old_fds[1]);
				}
				if (i_pipe_cmds != pipe_cmds_num - 1) // if there is a next command
				{
					close(new_fds[0]);
					dup2(new_fds[1], 1); // TODO: if failed, call fatal()
					close(new_fds[1]);
				}
				
				if (strcmp(args[0], "cd") == 0)
				{
					if (cmd_size != 2)
					{
						write(2, "error: cd: bad arguments\n", 25);
						exit(1);
					}
					else
					{
						if (chdir(args[1]) == -1)
						{
							write(2, "error: cd: cannot change directory to ", 38);
							write(2, args[1], strlen(args[1]));
							write(2, "\n", 1);
							exit(1);
						}
						else
							exit(0);
					}
				}
				else if (execve(args[0], args, env) == -1)
				{
					write(2, "error: cannot execute ", 22);
					write(2, args[0], strlen(args[0]));
					write(2, "\n", 1);
					exit(1);
				}
			}
			else
			{
				pids[i_pipe_cmds] = pid;
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

			// go to next pipe-seperated command
			i_pipe_cmds++;

			i_argv++; // escape the "|" or ";" argument (this is where we left-off after copying from argv to args)
		}

		if (pipe_cmds_num > 1) // if there are multiple commands
		{
			close(old_fds[0]);
			close(old_fds[1]);
		}

		// wait for all child processes to finish
		for (int i = 0; i < pipe_cmds_num; i++)
			waitpid(pids[i], NULL, 0); // TODO: if failed, call fatal()

		// go to next semicolon-seperated command
		i_semicolon_cmds++;
	}

	return (0);
}