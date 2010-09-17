/*
 * =====================================================================================
 *
 *       Filename:  test_stats.c
 *
 *    Description:  test suite for libbeanstalkproto stats commands
 *
 *        Version:  1.0
 *        Created:  05/27/2010 02:09:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Roey Berman, roey.berman@gmail.com
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <check.h>

#include "beanstalkproto.h"
#define  PATH_TO(file) "response_samples/" file

char *open_stats( const char *filename, char **error_str )
{
    FILE *pFile;
    char *buffer;
    long lSize;

    if ( ( pFile = fopen( filename, "r" ) ) == NULL )
        *error_str = strdup("File error");

    else {
        // obtain file size:
        fseek(pFile , 0 , SEEK_END);
        lSize = ftell(pFile);
        rewind (pFile);

        // allocate memory to contain the whole file:
        if ( ( buffer = (char *) malloc(sizeof(char) * lSize) ) == NULL )
            *error_str = strdup("Memory error");
        else {
            // copy the file into the buffer:
            if ( fread(buffer, 1, lSize, pFile) != lSize )
                *error_str = strdup("Read error");
            else {
                return buffer;
            }
            free(buffer);
        }
        fclose(pFile);
    }

    return NULL;
}

START_TEST(test_bsp_parse_job_stats)
{
    char *buffer, *error_str;
    bsc_job_stats  *job;
    uint32_t bytes = 0, exp_id = 4, exp_pri = 1, exp_age = 786623, exp_delay = 2, exp_ttr = 3, exp_time_left = 0;
    char     *exp_tube = "default";
    bsc_job_state exp_state = BSC_JOB_STATE_READY;
    bsc_response_t got_t, exp_t = BSC_RES_OK;

    if ( ( buffer = open_stats(PATH_TO("stats-job.response"), &error_str) ) != NULL ) {
        got_t = bsp_get_stats_job_res( buffer, &bytes );
        job = bsp_parse_job_stats(strchr(buffer, '\n')+1);
        fail_unless( job != NULL,                       "bsp_get_job_stats_res(job != NULL)" );
        fail_unless( exp_t == got_t,                    "bsp_get_job_stats_res(code),  got: %u,  expected: %u",      got_t,  exp_t );
        fail_unless( exp_id == job->id,                 "bsp_get_job_stats_res(id),  got: %u,  expected: %u",        job->id,  exp_id );
        fail_unless( exp_pri == job->pri,               "bsp_get_job_stats_res(pri),  got: %u,  expected: %u",       job->pri,  exp_pri );
        fail_unless( exp_age == job->age,               "bsp_get_job_stats_res(age),  got: %u,  expected: %u",       job->age,  exp_age );
        fail_unless( exp_delay == job->delay,           "bsp_get_job_stats_res(delay),  got: %u,  expected: %u",     job->delay,  exp_delay );
        fail_unless( exp_ttr == job->ttr,               "bsp_get_job_stats_res(ttr),  got: %u,  expected: %u",       job->ttr,  exp_ttr );
        fail_unless( exp_time_left == job->time_left,   "bsp_get_job_stats_res(time_left),  got: %d,  expected: %d", job->time_left,  exp_time_left );
        fail_unless( exp_state == job->state,           "bsp_get_job_stats_res(state),  got: %d,  expected: %d",     job->state,  exp_state );
        fail_unless( strcmp(exp_tube, job->tube) == 0,  "bsp_get_job_stats_res(tube),  got: %s,  expected: %s",      job->tube,  exp_tube );
        bsc_job_stats_free(job);
        free(buffer);
    }
    else {
        fprintf(stderr, "skipped test: (%s)\n", error_str);
    }
}
END_TEST

START_TEST(test_bsp_parse_tube_stats)
{
    char *buffer, *error_str;
    bsc_tube_stats *tube;
    char     *exp_name = "default";
    uint32_t bytes = 0, exp_current_jobs_urgent = 193, exp_current_jobs_ready = 193, exp_current_jobs_reserved = 0,
             exp_current_jobs_delayed = 0, exp_current_jobs_buried = 0, exp_total_jobs = 193,
             exp_current_using = 1, exp_current_watching = 1, exp_current_waiting = 0,
             exp_cmd_pause_tube = 0, exp_pause = 0, exp_pause_time_left = 0;

    bsc_job_state exp_state = BSC_JOB_STATE_READY;
    bsc_response_t got_t, exp_t = BSC_RES_OK;

    if ( ( buffer = open_stats(PATH_TO("stats-tube.response"), &error_str) ) != NULL ) {
        got_t = bsp_get_stats_tube_res( buffer, &bytes );
        tube = bsp_parse_tube_stats(strchr(buffer, '\n')+1);
        fail_unless( tube != NULL,                              "bsp_get_tube_stats_res(tube != NULL)" );
        fail_unless( exp_t == got_t,                            "bsp_get_tube_stats_res(code),  got: %u,  expected: %u",      got_t,  exp_t );

        fail_unless( exp_current_jobs_urgent == tube->current_jobs_urgent,
            "bsp_get_tube_stats_res(current_jobs_urgent),  got: %u,  expected: %u",        tube->current_jobs_urgent,  exp_current_jobs_urgent );
        fail_unless( exp_current_jobs_ready == tube->current_jobs_ready,
            "bsp_get_tube_stats_res(current_jobs_ready),  got: %u,  expected: %u",        tube->current_jobs_ready,  exp_current_jobs_ready );
        fail_unless( exp_current_jobs_reserved == tube->current_jobs_reserved,
            "bsp_get_tube_stats_res(current_jobs_reserved),  got: %u,  expected: %u",        tube->current_jobs_reserved,  exp_current_jobs_reserved );
        fail_unless( exp_current_jobs_delayed == tube->current_jobs_delayed,
            "bsp_get_tube_stats_res(current_jobs_delayed),  got: %u,  expected: %u",        tube->current_jobs_delayed,  exp_current_jobs_delayed );
        fail_unless( exp_current_jobs_buried == tube->current_jobs_buried,
            "bsp_get_tube_stats_res(current_jobs_buried),  got: %u,  expected: %u",        tube->current_jobs_buried,  exp_current_jobs_buried );
        fail_unless( exp_total_jobs == tube->total_jobs,
            "bsp_get_tube_stats_res(total_jobs),  got: %u,  expected: %u",        tube->total_jobs,  exp_total_jobs );
        fail_unless( exp_current_using == tube->current_using,
            "bsp_get_tube_stats_res(current_using),  got: %u,  expected: %u",        tube->current_using,  exp_current_using );
        fail_unless( exp_current_watching == tube->current_watching,
            "bsp_get_tube_stats_res(current_watching),  got: %u,  expected: %u",        tube->current_watching,  exp_current_watching );
        fail_unless( exp_current_waiting == tube->current_waiting,
            "bsp_get_tube_stats_res(current_waiting),  got: %u,  expected: %u",        tube->current_waiting,  exp_current_waiting );
        fail_unless( exp_cmd_pause_tube == tube->cmd_pause_tube,
            "bsp_get_tube_stats_res(cmd_pause_tube),  got: %u,  expected: %u",        tube->cmd_pause_tube,  exp_cmd_pause_tube );
        fail_unless( exp_pause == tube->pause,
            "bsp_get_tube_stats_res(pause),  got: %u,  expected: %u",        tube->pause,  exp_pause );
        fail_unless( exp_pause_time_left == tube->pause_time_left,
            "bsp_get_tube_stats_res(pause_time_left),  got: %u,  expected: %u",        tube->pause_time_left,  exp_pause_time_left );
        fail_unless( strcmp(exp_name, tube->name) == 0,  "bsp_get_tube_stats_res(tube),  got: %s,  expected: %s",      tube->name,  exp_name );
        bsc_tube_stats_free(tube);
        free(buffer);
    }
    else {
        fprintf(stderr, "skipped test: (%s)\n", error_str);
    }
}
END_TEST

START_TEST(test_bsp_parse_server_stats)
{
    char *buffer, *error_str;
    bsc_server_stats *server;
    char     *exp_version = "1.4.5";
    uint32_t bytes = 0, exp_current_jobs_urgent = 193, exp_current_jobs_ready = 193, exp_current_jobs_reserved = 0,
             exp_current_jobs_delayed = 0, exp_current_jobs_buried = 0, exp_cmd_put = 193,
             exp_cmd_peek = 0, exp_cmd_peek_ready = 0, exp_cmd_peek_delayed = 0, exp_cmd_peek_buried = 0,
             exp_cmd_reserve = 1, exp_cmd_reserve_with_timeout = 0, exp_cmd_delete = 0, exp_cmd_release = 0,
             exp_cmd_use = 193, exp_cmd_watch = 0, exp_cmd_ignore = 0, exp_cmd_bury = 0,
             exp_cmd_kick = 0, exp_cmd_touch = 0, exp_cmd_stats = 25, exp_cmd_stats_job = 31,
             exp_cmd_stats_tube = 25, exp_cmd_list_tubes = 11, exp_cmd_list_tube_used = 1, exp_cmd_list_tubes_watched = 1,
             exp_cmd_pause_tube = 0, exp_job_timeouts = 1, exp_total_jobs = 193, exp_max_job_size = 65535,
             exp_current_tubes = 1, exp_current_connections = 1, exp_current_producers = 0, exp_current_workers = 0,
             exp_current_waiting = 0, exp_total_connections = 240, exp_pid = 9015,
             exp_uptime = 791125, exp_binlog_oldest_index = 0, exp_binlog_current_index = 0, exp_binlog_max_size = 10485760;
    double exp_rusage_utime = 0.039997, exp_rusage_stime = 0.133324;

    bsc_job_state exp_state = BSC_JOB_STATE_READY;
    bsc_response_t got_t, exp_t = BSC_RES_OK;

    if ( ( buffer = open_stats(PATH_TO("stats.response"), &error_str) ) != NULL ) {
        got_t = bsp_get_stats_res( buffer, &bytes );
        server = bsp_parse_server_stats(strchr(buffer, '\n')+1);
        fail_unless( server != NULL,                            "bsp_get_stats_res(server != NULL)" );
        fail_unless( exp_t == got_t,                            "bsp_get_stats_res(code),  got: %u,  expected: %u",      got_t,  exp_t );
        fail_unless( server->current_jobs_urgent == exp_current_jobs_urgent,
            "bsp_get_stats_res(current_jobs_urgent), got: %u, expected: %u", server->current_jobs_urgent, exp_current_jobs_urgent );
        fail_unless( server->current_jobs_ready == exp_current_jobs_ready,
            "bsp_get_stats_res(current_jobs_ready), got: %u, expected: %u", server->current_jobs_ready, exp_current_jobs_ready );
        fail_unless( server->current_jobs_reserved == exp_current_jobs_reserved,
            "bsp_get_stats_res(current_jobs_reserved), got: %u, expected: %u", server->current_jobs_reserved, exp_current_jobs_reserved );
        fail_unless( server->current_jobs_delayed == exp_current_jobs_delayed,
            "bsp_get_stats_res(current_jobs_delayed), got: %u, expected: %u", server->current_jobs_delayed, exp_current_jobs_delayed );
        fail_unless( server->current_jobs_buried == exp_current_jobs_buried,
            "bsp_get_stats_res(current_jobs_buried), got: %u, expected: %u", server->current_jobs_buried, exp_current_jobs_buried );
        fail_unless( server->cmd_put == exp_cmd_put,
            "bsp_get_stats_res(cmd_put), got: %u, expected: %u", server->cmd_put, exp_cmd_put );
        fail_unless( server->cmd_peek == exp_cmd_peek,
            "bsp_get_stats_res(cmd_peek), got: %u, expected: %u", server->cmd_peek, exp_cmd_peek );
        fail_unless( server->cmd_peek_ready == exp_cmd_peek_ready,
            "bsp_get_stats_res(cmd_peek_ready), got: %u, expected: %u", server->cmd_peek_ready, exp_cmd_peek_ready );
        fail_unless( server->cmd_peek_delayed == exp_cmd_peek_delayed,
            "bsp_get_stats_res(cmd_peek_delayed), got: %u, expected: %u", server->cmd_peek_delayed, exp_cmd_peek_delayed );
        fail_unless( server->cmd_peek_buried == exp_cmd_peek_buried,
            "bsp_get_stats_res(cmd_peek_buried), got: %u, expected: %u", server->cmd_peek_buried, exp_cmd_peek_buried );
        fail_unless( server->cmd_reserve == exp_cmd_reserve,
            "bsp_get_stats_res(cmd_reserve), got: %u, expected: %u", server->cmd_reserve, exp_cmd_reserve );
        fail_unless( server->cmd_reserve_with_timeout == exp_cmd_reserve_with_timeout,
            "bsp_get_stats_res(cmd_reserve_with_timeout), got: %u, expected: %u", server->cmd_reserve_with_timeout, exp_cmd_reserve_with_timeout );
        fail_unless( server->cmd_delete == exp_cmd_delete,
            "bsp_get_stats_res(cmd_delete), got: %u, expected: %u", server->cmd_delete, exp_cmd_delete );
        fail_unless( server->cmd_release == exp_cmd_release,
            "bsp_get_stats_res(cmd_release), got: %u, expected: %u", server->cmd_release, exp_cmd_release );
        fail_unless( server->cmd_use == exp_cmd_use,
            "bsp_get_stats_res(cmd_use), got: %u, expected: %u", server->cmd_use, exp_cmd_use );
        fail_unless( server->cmd_watch == exp_cmd_watch,
            "bsp_get_stats_res(cmd_watch), got: %u, expected: %u", server->cmd_watch, exp_cmd_watch );
        fail_unless( server->cmd_ignore == exp_cmd_ignore,
            "bsp_get_stats_res(cmd_ignore), got: %u, expected: %u", server->cmd_ignore, exp_cmd_ignore );
        fail_unless( server->cmd_bury == exp_cmd_bury,
            "bsp_get_stats_res(cmd_bury), got: %u, expected: %u", server->cmd_bury, exp_cmd_bury );
        fail_unless( server->cmd_kick == exp_cmd_kick,
            "bsp_get_stats_res(cmd_kick), got: %u, expected: %u", server->cmd_kick, exp_cmd_kick );
        fail_unless( server->cmd_touch == exp_cmd_touch,
            "bsp_get_stats_res(cmd_touch), got: %u, expected: %u", server->cmd_touch, exp_cmd_touch );
        fail_unless( server->cmd_stats == exp_cmd_stats,
            "bsp_get_stats_res(cmd_stats), got: %u, expected: %u", server->cmd_stats, exp_cmd_stats );
        fail_unless( server->cmd_stats_job == exp_cmd_stats_job,
            "bsp_get_stats_res(cmd_stats_job), got: %u, expected: %u", server->cmd_stats_job, exp_cmd_stats_job );
        fail_unless( server->cmd_stats_tube == exp_cmd_stats_tube,
            "bsp_get_stats_res(cmd_stats_tube), got: %u, expected: %u", server->cmd_stats_tube, exp_cmd_stats_tube );
        fail_unless( server->cmd_list_tubes == exp_cmd_list_tubes,
            "bsp_get_stats_res(cmd_list_tubes), got: %u, expected: %u", server->cmd_list_tubes, exp_cmd_list_tubes );
        fail_unless( server->cmd_list_tube_used == exp_cmd_list_tube_used,
            "bsp_get_stats_res(cmd_list_tube_used), got: %u, expected: %u", server->cmd_list_tube_used, exp_cmd_list_tube_used );
        fail_unless( server->cmd_list_tubes_watched == exp_cmd_list_tubes_watched,
            "bsp_get_stats_res(cmd_list_tubes_watched), got: %u, expected: %u", server->cmd_list_tubes_watched, exp_cmd_list_tubes_watched );
        fail_unless( server->cmd_pause_tube == exp_cmd_pause_tube,
            "bsp_get_stats_res(cmd_pause_tube), got: %u, expected: %u", server->cmd_pause_tube, exp_cmd_pause_tube );
        fail_unless( server->job_timeouts == exp_job_timeouts,
            "bsp_get_stats_res(job_timeouts), got: %u, expected: %u", server->job_timeouts, exp_job_timeouts );
        fail_unless( server->total_jobs == exp_total_jobs,
            "bsp_get_stats_res(total_jobs), got: %u, expected: %u", server->total_jobs, exp_total_jobs );
        fail_unless( server->max_job_size == exp_max_job_size,
            "bsp_get_stats_res(max_job_size), got: %u, expected: %u", server->max_job_size, exp_max_job_size );
        fail_unless( server->current_tubes == exp_current_tubes,
            "bsp_get_stats_res(current_tubes), got: %u, expected: %u", server->current_tubes, exp_current_tubes );
        fail_unless( server->current_connections == exp_current_connections,
            "bsp_get_stats_res(current_connections), got: %u, expected: %u", server->current_connections, exp_current_connections );
        fail_unless( server->current_producers == exp_current_producers,
            "bsp_get_stats_res(current_producers), got: %u, expected: %u", server->current_producers, exp_current_producers );
        fail_unless( server->current_workers == exp_current_workers,
            "bsp_get_stats_res(current_workers), got: %u, expected: %u", server->current_workers, exp_current_workers );
        fail_unless( server->current_waiting == exp_current_waiting,
            "bsp_get_stats_res(current_waiting), got: %u, expected: %u", server->current_waiting, exp_current_waiting );
        fail_unless( server->total_connections == exp_total_connections,
            "bsp_get_stats_res(total_connections), got: %u, expected: %u", server->total_connections, exp_total_connections );
        fail_unless( server->pid == exp_pid,
            "bsp_get_stats_res(pid), got: %u, expected: %u", server->pid, exp_pid );
        fail_unless( strcmp(exp_version, server->version) == 0,  "bsp_get_server_stats_res(server),  got: %s,  expected: %s",      server->version,  exp_version );
        fail_unless( server->rusage_utime == exp_rusage_utime,
            "bsp_get_stats_res(rusage_utime), got: %f, expected: %f", server->rusage_utime, exp_rusage_utime );
        fail_unless( server->rusage_stime == exp_rusage_stime,
            "bsp_get_stats_res(rusage_stime), got: %f, expected: %f", server->rusage_stime, exp_rusage_stime );
        fail_unless( server->uptime == exp_uptime,
            "bsp_get_stats_res(uptime), got: %u, expected: %u", server->uptime, exp_uptime );
        fail_unless( server->binlog_oldest_index == exp_binlog_oldest_index,
            "bsp_get_stats_res(binlog_oldest_index), got: %u, expected: %u", server->binlog_oldest_index, exp_binlog_oldest_index );
        fail_unless( server->binlog_current_index == exp_binlog_current_index,
            "bsp_get_stats_res(binlog_current_index), got: %u, expected: %u", server->binlog_current_index, exp_binlog_current_index );
        fail_unless( server->binlog_max_size == exp_binlog_max_size,
            "bsp_get_stats_res(binlog_max_size), got: %u, expected: %u", server->binlog_max_size, exp_binlog_max_size );

        bsc_server_stats_free(server);
        free(buffer);
    }
    else {
        fprintf(stderr, "skipped test: (%s)\n", error_str);
    }
}
END_TEST

START_TEST(test_bsp_parse_tubes_list)
{
    char *buffer, *error_str, **tubes;
    bsc_response_t got_t, exp_t = BSC_RES_OK;
    uint32_t bytes = 0;

    if ( ( buffer = open_stats(PATH_TO("list-tubes.response"), &error_str) ) != NULL ) {
        got_t = bsp_get_list_tubes_res( buffer, &bytes );
        tubes = bsp_parse_tube_list(strchr(buffer, '\n')+1);
        fail_unless( tubes != NULL, "bsp_get_list_tubes_res(tubes != NULL)" );
        fail_unless( strcmp(tubes[0], "default") == 0, "bsp_parse_tube_list -> got default tube" );
        fail_unless( strcmp(tubes[1], "baba") == 0, "bsp_parse_tube_list -> got 'baba' tube" );
        free(buffer);
    }
    else {
        fprintf(stderr, "skipped test: (%s)\n", error_str);
    }
}
END_TEST

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create(__FILE__);

    tcase_add_test(tc, test_bsp_parse_job_stats);
    tcase_add_test(tc, test_bsp_parse_tube_stats);
    tcase_add_test(tc, test_bsp_parse_server_stats);
    tcase_add_test(tc, test_bsp_parse_tubes_list);

    suite_add_tcase(s, tc);
    return s;
}

int main()
{
    SRunner *sr;
    Suite *s;
    int failed;

    s = local_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);

    failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

