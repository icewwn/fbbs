#include <sys/types.h>
#include <unistd.h>

#include "fbbs/fbbs.h"
#include "fbbs/session.h"

bbs_session_t session;

int get_online_count(db_conn_t *c)
{
	int ret = -1;
	db_res_t *res = db_exec(c, "SELECT count(*) FROM sessions");
	if (db_res_status(res) == DBRES_TUPLES_OK)
		ret = db_get_integer(res, 0, 0);
	db_clear(res);
	return ret;
}

session_id_t session_new_id(void)
{
	db_res_t *res = db_exec_query(env.d, true,
			"SELECT nextval('sessions_id_seq')");
	if (!res)
		return 0;

	session_id_t sid = db_get_session_id(res, 0, 0);
	db_clear(res);
	return sid;
}

session_id_t session_new(const char *key, session_id_t sid, user_id_t uid,
		const char *ip_addr, bool is_web, bool is_secure)
{
	int pid = getpid();

	db_res_t *res = db_cmd("INSERT INTO sessions"
			" (id, session_key, user_id, pid, ip_addr, web, secure)"
			" VALUES (%"DBIdSID", %s, %"DBIdUID", %d, %s, %d, %d)",
			sid ? sid : session_new_id(),
			key, uid, pid, ip_addr, is_web, is_secure);
	if (!res)
		return 0;
	db_clear(res);
	return sid;
}

int set_idle_time(session_id_t sid, fb_time_t t)
{
	char sid_buf[26], time_buf[26];
	snprintf(sid_buf, sizeof(sid_buf), "%"PRIdSID, sid);
	snprintf(time_buf, sizeof(time_buf), "%"PRIdFBT, t);

	mdb_cmd("ZADD idle %s %s", sid, t);
}
