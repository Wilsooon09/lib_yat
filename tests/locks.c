#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h> /* for waitpid() */

#include "tests.h"
#include "yat.h"


TESTCASE(not_lock_fmlp_be, GSN_EDF | PSN_EDF | P_FP,
	 "don't let best-effort tasks lock FMLP semaphores")
{
	int fd, od;

	SYSCALL( fd = open(".fmlp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( od = open_fmlp_sem(fd, 0) );

	/* BE tasks may not lock FMLP semaphores */
	SYSCALL_FAILS(EPERM, yat_lock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, yat_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );

}

TESTCASE(not_lock_srp_be, NONE,
	 "don't let best-effort tasks open SRP semaphores")
{
	int fd, od;

	SYSCALL( fd = open(".srp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	/* BE tasks may not open SRP semaphores */

	SYSCALL_FAILS(EPERM, od = open_srp_sem(fd, 0) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".srp_locks") );

}

TESTCASE(lock_srp, NONE,
	 "SRP acquisition and release")
{
	int fd, od;

	SYSCALL( fd = open(".srp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(ms2ns(10), ms2ns(100), 0) );
	SYSCALL( task_mode(YAT_RT_TASK) );

	SYSCALL( od = open_srp_sem(fd, 0) );

	SYSCALL( yat_lock(od) );
	SYSCALL( yat_unlock(od) );

	SYSCALL( yat_lock(od) );
	SYSCALL( yat_unlock(od) );

	SYSCALL( yat_lock(od) );
	SYSCALL( yat_unlock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, yat_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".srp_locks") );
}


TESTCASE(lock_fmlp, PSN_EDF | GSN_EDF | P_FP,
	 "FMLP acquisition and release")
{
	int fd, od;

	SYSCALL( fd = open(".fmlp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(ms2ns(10), ms2ns(100), 0) );
	SYSCALL( task_mode(YAT_RT_TASK) );

	SYSCALL( od = open_fmlp_sem(fd, 0) );

	SYSCALL( yat_lock(od) );
	SYSCALL( yat_unlock(od) );

	SYSCALL( yat_lock(od) );
	SYSCALL( yat_unlock(od) );

	SYSCALL( yat_lock(od) );
	SYSCALL( yat_unlock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, yat_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );
}

TESTCASE(lock_dflp, P_FP,
	 "DFLP acquisition and release")
{
	int fd, od, cpu = 1;

	SYSCALL( fd = open(".dflp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(ms2ns(10), ms2ns(100), 0) );
	SYSCALL( task_mode(YAT_RT_TASK) );

	SYSCALL( od = open_dflp_sem(fd, 0, cpu) );

	SYSCALL( yat_lock(od) );
	SYSCALL( yat_unlock(od) );

	SYSCALL( yat_lock(od) );
	SYSCALL( yat_unlock(od) );

	SYSCALL( yat_lock(od) );
	SYSCALL( yat_unlock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, yat_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".dflp_locks") );
}

TESTCASE(dflp_bad_cpu, P_FP,
	 "DFLP reject bad CPU parameter")
{
	int fd, cpu = 0xbadca5e;

	SYSCALL( fd = open(".dflp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(ms2ns(10), ms2ns(100), 0) );
	SYSCALL( task_mode(YAT_RT_TASK) );

	SYSCALL_FAILS(EINVAL, open_dflp_sem(fd, 0, cpu) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".dflp_locks") );
}

TESTCASE(srp_lock_mode_change, NONE,
	 "SRP task becomes non-RT task while holding lock")
{
	int fd, od;

	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  =  ms2ns(10000);
	params.period     = ms2ns(100000);
	params.relative_deadline = params.period;

	SYSCALL( fd = open(".locks", O_RDONLY | O_CREAT, S_IRUSR) );

	params.priority = YAT_LOWEST_PRIORITY;
	SYSCALL( set_rt_task_param(gettid(), &params) );
	SYSCALL( be_migrate_to_cpu(params.cpu) );
	SYSCALL( task_mode(YAT_RT_TASK) );

	SYSCALL( od = open_srp_sem(fd, 0) );

	SYSCALL( yat_lock(od) );

	SYSCALL( task_mode(BACKGROUND_TASK) );

	be_migrate_to_cpu(1);

	SYSCALL( yat_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".locks") );
}

TESTCASE(dflp_lock_mode_change, P_FP,
	 "DFLP task becomes non-RT task while holding lock")
{
	int fd, od, cpu = 0;

	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 1;
	params.exec_cost  =  ms2ns(10000);
	params.period     = ms2ns(100000);
	params.relative_deadline = params.period;

	SYSCALL( fd = open(".locks", O_RDONLY | O_CREAT, S_IRUSR) );

	params.priority = YAT_LOWEST_PRIORITY;
	SYSCALL( set_rt_task_param(gettid(), &params) );
	SYSCALL( be_migrate_to_cpu(params.cpu) );
	SYSCALL( task_mode(YAT_RT_TASK) );

	SYSCALL( od = open_dflp_sem(fd, 0, cpu) );

	SYSCALL( yat_lock(od) );

	SYSCALL( task_mode(BACKGROUND_TASK) );

	be_migrate_to_cpu(2);

	SYSCALL( yat_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".locks") );
}

TESTCASE(fmlp_lock_mode_change, P_FP | PSN_EDF | GSN_EDF,
	 "FMLP task becomes non-RT task while holding lock")
{
	int fd, od;

	int child, status;

	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  =  ms2ns(10000);
	params.period     = ms2ns(100000);
	params.relative_deadline = params.period;

	SYSCALL( fd = open(".locks", O_RDONLY | O_CREAT, S_IRUSR) );


	child = FORK_TASK(
		params.priority = YAT_LOWEST_PRIORITY;
		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(YAT_RT_TASK) );

		SYSCALL( od = open_fmlp_sem(fd, 0) );

		SYSCALL( yat_lock(od) );

		SYSCALL( task_mode(BACKGROUND_TASK) );

		SYSCALL( yat_unlock(od) );

		SYSCALL( od_close(od) );

		exit(0);
		);

	SYSCALL( waitpid(child, &status, 0) );
	ASSERT( WIFEXITED(status) );
	ASSERT( WEXITSTATUS(status) == 0 );

	SYSCALL( close(fd) );

	SYSCALL( remove(".locks") );
}

TESTCASE(srp_lock_teardown, NONE,
	 "SRP task exits while holding lock")
{
	int fd, od;

	int child, status;

	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  =  ms2ns(10000);
	params.period     = ms2ns(100000);
	params.relative_deadline = params.period;

	SYSCALL( fd = open(".locks", O_RDONLY | O_CREAT, S_IRUSR) );

	exit(0);

	child = FORK_TASK(
		params.priority = YAT_LOWEST_PRIORITY;
		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(YAT_RT_TASK) );

		SYSCALL( od = open_srp_sem(fd, 0) );

		SYSCALL( yat_lock(od) );
		exit(123);
		);

	SYSCALL( waitpid(child, &status, 0) );
	ASSERT( WIFEXITED(status) );
	ASSERT( WEXITSTATUS(status) == 123 );

	SYSCALL( close(fd) );

	SYSCALL( remove(".locks") );
}

TESTCASE(fmlp_lock_teardown, P_FP | PSN_EDF | GSN_EDF,
	 "FMLP task exits while holding lock")
{
	int fd, od;

	int child, status;

	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  =  ms2ns(10000);
	params.period     = ms2ns(100000);
	params.relative_deadline = params.period;

	SYSCALL( fd = open(".locks", O_RDONLY | O_CREAT, S_IRUSR) );

	exit(0);

	child = FORK_TASK(
		params.priority = YAT_LOWEST_PRIORITY;
		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(YAT_RT_TASK) );

		SYSCALL( od = open_fmlp_sem(fd, 0) );

		SYSCALL( yat_lock(od) );
		exit(123);
		);

	SYSCALL( waitpid(child, &status, 0) );
	ASSERT( WIFEXITED(status) );
	ASSERT( WEXITSTATUS(status) == 123 );

	SYSCALL( close(fd) );

	SYSCALL( remove(".locks") );
}

TESTCASE(dflp_lock_teardown, P_FP,
	 "DFLP task exits while holding lock")
{
	int fd, od, cpu = 0;

	int child, status;

	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 1;
	params.exec_cost  =  ms2ns(10000);
	params.period     = ms2ns(100000);
	params.relative_deadline = params.period;

	SYSCALL( fd = open(".locks", O_RDONLY | O_CREAT, S_IRUSR) );

	exit(0);

	child = FORK_TASK(
		params.priority = YAT_LOWEST_PRIORITY;
		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(YAT_RT_TASK) );

		SYSCALL( od = open_dflp_sem(fd, 0, cpu) );

		SYSCALL( yat_lock(od) );
		exit(123);
		);

	SYSCALL( waitpid(child, &status, 0) );
	ASSERT( WIFEXITED(status) );
	ASSERT( WEXITSTATUS(status) == 123 );

	SYSCALL( close(fd) );

	SYSCALL( remove(".locks") );
}

