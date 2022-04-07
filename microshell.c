/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mhaddi <mhaddi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/04/04 14:36:47 by mhaddi            #+#    #+#             */
/*   Updated: 2022/04/07 12:43:40 by mhaddi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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

	// loop through the commands seperated by ;
	int i_argv = 1;
	int i_semicolon_cmds = 0;
	while (i_semicolon_cmds < semicolon_cmds_num)
	{
		// loop through the commands seperated by |
		int pipe_cmds_num = semicolon_cmds[i_semicolon_cmds];
		pid_t pids[pipe_cmds_num];
		int new_fds[2], old_fds[2];
		int i_pipe_cmds = 0;
		while (i_pipe_cmds < pipe_cmds_num)
		{
			// create args for execve
			int cmd_size = get_command_size(i_argv, argc, argv);
			char *args[cmd_size + 1];
			int i = 0;
			while (i < cmd_size)
			{
				args[i] = argv[i_argv];
				i_argv++;
				i++;
			}
			args[i] = NULL;

			// pipes
			if (i_pipe_cmds != pipe_cmds_num - 1) // if there is a next command
				pipe(new_fds);
			pid_t pid = fork();
			if (pid == 0)
			{
				if (i_pipe_cmds != 0) // if there is a previous command
				{
					dup2(old_fds[0], 0);
					close(old_fds[0]);
					close(old_fds[1]);
				}
				if (i_pipe_cmds != pipe_cmds_num - 1) // if there is a next command
				{
					close(new_fds[0]);
					dup2(new_fds[1], 1);
					close(new_fds[1]);
				}
				
				if (strcmp(args[0], "cd") == 0)
				{
					if (chdir(args[1]) == -1)
						perror("cd");
				}
				else if (execve(args[0], args, env) == -1)
					printf("%s: command not found\n", args[0]);
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

			i_argv++;
			i_pipe_cmds++;
		}
		if (pipe_cmds_num > 1) // if there are multiple commands
		{
			close(old_fds[0]);
			close(old_fds[1]);
		}
		for (int i = 0; i < pipe_cmds_num; i++)
			waitpid(pids[i], NULL, 0);

		i_semicolon_cmds++;
	}

	return (0);
}