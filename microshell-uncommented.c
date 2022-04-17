/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell-uncommented.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mhaddi <mhaddi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/04/17 18:50:54 by mhaddi            #+#    #+#             */
/*   Updated: 2022/04/17 18:50:59 by mhaddi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int count_pipe_seperated_cmds(int i_argv, int argc, char **argv);
int get_command_size(int i_argv, int argc, char **argv);
void cd(int cmd_size, char **args);
void fatal();

int main(int argc, char **argv, char **env)
{
	int i_argv = 1;
	while (i_argv < argc)
	{
		int pipe_cmds_num = count_pipe_seperated_cmds(i_argv, argc, argv);
		pid_t pids[pipe_cmds_num];
		int i_pipe_cmds = 0;
		int new_fds[2], old_fds[2];

		while (i_pipe_cmds < pipe_cmds_num)
		{
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

			if (i_pipe_cmds != pipe_cmds_num - 1)
				if (pipe(new_fds) == -1) fatal();
			pid_t pid = fork();
			if (pid == -1) fatal();
			if (pid == 0)
			{
				if (i_pipe_cmds != 0)
				{
					if (dup2(old_fds[0], 0) == -1) fatal();
					close(old_fds[0]);
					close(old_fds[1]);
				}
				if (i_pipe_cmds != pipe_cmds_num - 1)
				{
					close(new_fds[0]);
					if (dup2(new_fds[1], 1) == -1) fatal();
					close(new_fds[1]);
				}
				
				if (strcmp(args[0], "cd"))
				{
					if (execve(args[0], args, env) == -1)
					{
						write(2, "error: cannot execute ", 22);
						write(2, args[0], strlen(args[0]));
						write(2, "\n", 1);
						exit(1);
					}
				}
				else cd(cmd_size, args);
			}
			else
			{
				pids[i_pipe_cmds] = pid;
				if (i_pipe_cmds != 0)
				{
					close(old_fds[0]);
					close(old_fds[1]);
				}
				if (i_pipe_cmds != pipe_cmds_num - 1)
				{
					for (int i = 0; i < 2; i++)
						old_fds[i] = new_fds[i];
				}
			}

			i_argv++;
			i_pipe_cmds++;
		}
		if (pipe_cmds_num > 1)
		{
			close(old_fds[0]);
			close(old_fds[1]);
		}

		for (int i = 0; i < pipe_cmds_num; i++)
			if (waitpid(pids[i], NULL, 0) == -1) fatal();
	}

	return (0);
}

void cd(int cmd_size, char **args)
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