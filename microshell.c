/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mhaddi <mhaddi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/04/04 14:36:47 by mhaddi            #+#    #+#             */
/*   Updated: 2022/04/07 12:01:20 by mhaddi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv, char **env)
{
	if (argc == 1)
		return (0);

	int argv_i = 1;
	int count_sep_cmds = 1;
	while (argv_i < argc)
	{
		if (strcmp(argv[argv_i], ";") == 0)
			count_sep_cmds++;
		argv_i++;
	}

	int sep_cmds_num[count_sep_cmds];

	argv_i = 1;
	int sep_cmds_i = 0;
	while (sep_cmds_i < count_sep_cmds)
	{
		int count_pipe_cmds = 1;
		while (argv_i < argc)
		{
			if (strcmp(argv[argv_i], ";") == 0)
			{
				argv_i++;
				break;
			}
			if (strcmp(argv[argv_i], "|") == 0)
				count_pipe_cmds++;
			argv_i++;
		}
		sep_cmds_num[sep_cmds_i] = count_pipe_cmds;
		sep_cmds_i++;
	}

	argv_i = 1;
	sep_cmds_i = 0;
	while (sep_cmds_i < count_sep_cmds)
	{
		int pipe_cmds_num = sep_cmds_num[sep_cmds_i];
		int pipe_cmds_i = 0;
		int new_fds[2];
		int old_fds[2];
		int pids[pipe_cmds_num];
		while (pipe_cmds_i < pipe_cmds_num)
		{
			// exec args
			int cmd_size = 0;
			while (
				argv_i + cmd_size < argc
				&& strcmp(argv[argv_i + cmd_size], "|")
				&& strcmp(argv[argv_i + cmd_size], ";")
			)
			{
				cmd_size++;
			}
			char *args[cmd_size + 1];
			int args_i = 0;
			while (args_i < cmd_size)
			{
				args[args_i] = argv[argv_i];
				args_i++;
				argv_i++;
			}
			args[args_i] = NULL;

			// pipes
			if (pipe_cmds_i != pipe_cmds_num - 1) // if there is a next command
				pipe(new_fds);
			pid_t pid = fork();
			if (pid == 0)
			{
				if (pipe_cmds_i != 0) // if there is a previous command
				{
					dup2(old_fds[0], 0);
					close(old_fds[0]);
					close(old_fds[1]);
				}
				if (pipe_cmds_i != pipe_cmds_num - 1) // if there is a next command
				{
					close(new_fds[0]);
					dup2(new_fds[1], 1);
					close(new_fds[1]);
				}
				
				if (execve(args[0], args, env) == -1)
					printf("%s: command not found\n", args[0]);
			}
			else
			{
				pids[pipe_cmds_i] = pid;
				if (pipe_cmds_i != 0) // if there is a previous command
				{
					close(old_fds[0]);
					close(old_fds[1]);
				}
				if (pipe_cmds_i != pipe_cmds_num - 1) // if there is a next command
				{
					for (int i = 0; i < 2; i++)
						old_fds[i] = new_fds[i];
				}
			}

			argv_i++;
			pipe_cmds_i++;
		}
		if (pipe_cmds_num > 1) // if there are multiple commands
		{
			close(old_fds[0]);
			close(old_fds[1]);
		}
		for (int i = 0; i < pipe_cmds_num; i++)
			waitpid(pids[i], NULL, 0);

		sep_cmds_i++;
	}

	return (0);
}
