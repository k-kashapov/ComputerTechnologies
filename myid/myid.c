#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

static const int GROUPS_MAX = 160;

int PrintGrNames (int gr_num, gid_t *gr_list)
{
	if (gr_num < 0)
	{
		fprintf (stderr, "No groups passed");
		return -1;
	}

	printf ("groups=");

	struct group *grgid;

	for (int i = 0; i < gr_num - 1; i++)
	{
		grgid = getgrgid (gr_list[i]);
		if (!grgid)
		{
			perror ("Could not read group id!");
			return -1;
		}

		printf ("%d(%s), ", gr_list[i], grgid->gr_name);
	}

	grgid = getgrgid (gr_list[gr_num - 1]);
	if (!grgid)
	{
		perror ("Could not read group id");
		return -1;
	}

	printf ("%d(%s)", gr_list[gr_num - 1], grgid->gr_name);
	return 0;
}

int PrintEffIDName (uid_t id)
{
	struct passwd *id_pwd;

	printf ("%d", id);

	id_pwd = getpwuid (id);
	if (!id_pwd)
	{
		return 0;
	}
	else
	{
		printf ("(%s) ", id_pwd->pw_name);
	}

	return 0;
}

int ReadEffective (void)
{
	printf ("uid=");
	PrintEffIDName (getuid());

	printf ("gid=");
	PrintEffIDName (getegid());
	
	gid_t gr_list[GROUPS_MAX];

	int gr_num = getgroups (GROUPS_MAX, gr_list);

	if (gr_num < 0)
	{
		perror ("ERROR: could not get groups list");
		return -1;
	}

	PrintGrNames (gr_num, gr_list);

	return 0;
}

int ReadBase (const char *name)
{
	#ifdef _DEBUG
	printf ("arg = %s\n", name);
	#endif

	struct passwd *pwd;
	pwd = getpwnam (name);
	if (!pwd)
	{
		fprintf(stderr, "myid: \'%s\': no such user\n", name);
		return -1;
	}

	uid_t gid = pwd->pw_gid;

	printf ("uid=%d(%s) ", pwd->pw_uid, pwd->pw_name);
	printf ("gid=%d(%s) ", pwd->pw_gid, getpwuid (gid)->pw_name);

	gid_t gr_list[GROUPS_MAX];

	int gr_num = 0;
	
	getgrouplist (name, gid, gr_list, &gr_num);

	if (gr_num < 0)
	{
		perror ("ERROR: could not get groups list");
		return -1;
	}

	PrintGrNames (gr_num, gr_list);

	return 0;
}

int main (int argc, const char **argv)
{
	if (argc == 1)
	{
		return ReadEffective();
	}
	else
	{
		argc--;

		while (argc > 0)
		{
			int read_err = ReadBase (argv[argc--]);
			
			if (read_err)
			{
				return read_err;
			}
		}
	}

	return 0;
}
