#include "bbs.h"
#include "record.h"
#include "fbbs/fbbs.h"
#include "fbbs/helper.h"
#include "fbbs/session.h"
#include "fbbs/string.h"
#include "fbbs/terminal.h"
#include "fbbs/tui_list.h"

#define BBS_PAGESIZE (t_lines - 4)

enum {
	USRSORT_USERID = 0,
	USRSORT_NICK = 1,
	USRSORT_FROM = 2,
	USRSORT_STATUS = 3,
};

enum {
	REFRESH_TIME = 30,
};

typedef struct {
	time_t uptime;
	struct user_info **users; ///< Array of online users.
	int *ovrs;                ///< Array of online overriding users.
	int num;                  ///< Number of online users.
	int onum;                 ///< Number of online overriding users.
	int sort;                 ///< Sort method enum.
	bool ovr_only;            ///< True if show overriding users only.
	bool board;               ///< True if show users in current board only.
	bool show_note;           ///< True if show note to overriding users.
} online_users_t;

static int online_users_sort_userid(const void *usr1, const void *usr2)
{
	struct user_info * const *u1 = usr1;
	struct user_info * const *u2 = usr2;
	return strcasecmp((*u1)->userid, (*u2)->userid);
}

static int online_users_sort_nick(const void *usr1, const void *usr2)
{
	struct user_info * const *u1 = usr1;
	struct user_info * const *u2 = usr2;
	return strcasecmp((*u1)->username, (*u2)->username);
}

static int online_users_sort_from(const void *usr1, const void *usr2)
{
	struct user_info * const *u1 = usr1;
	struct user_info * const *u2 = usr2;
	return strcmp((*u1)->from, (*u2)->from);
}

static int online_users_sort_status(const void *usr1, const void *usr2)
{
	struct user_info * const *u1 = usr1;
	struct user_info * const *u2 = usr2;
	return (*u1)->mode - (*u2)->mode;
}

static const comparator_t comparators[] = {online_users_sort_userid,
		online_users_sort_nick, online_users_sort_from,
		online_users_sort_status};

static int online_users_init(online_users_t *up)
{
	memset(up, 0, sizeof(*up));

	up->users = malloc(sizeof(*up->users) * USHM_SIZE);
	if (!up->users)
		return -1;
	up->ovrs = malloc(sizeof(*up->ovrs) * USHM_SIZE);
	if (!up->ovrs) {
		free(up->users);
		return -1;
	}

	up->sort = USRSORT_USERID;

	up->uptime = 0;

	return 0;
}

static void online_users_swap(online_users_t *up, int a, int b)
{
	struct user_info *tmp = up->users[a];
	up->users[a] = up->users[b];
	up->users[b] = tmp;
}

static void set_num_rows(tui_list_t *p)
{
	online_users_t *up = p->data;
	p->all = up->ovr_only ? up->onum : up->num;
}

static tui_list_loader_t online_users_load(tui_list_t *p)
{
	online_users_t *up = p->data;

	time_t now = time(NULL);
	if (now < up->uptime + REFRESH_TIME)
		return p->all;
	up->uptime = now;

	resolve_utmp();

	up->num = 0;
	up->onum = 0;

	struct user_info *uin;
	int i;
	for (i = 0; i < USHM_SIZE; i++) {
		uin = utmpshm->uinfo + i;
		if (!uin->active || !uin->pid)
			continue;
		if (up->board && uin->currbrdnum != uinfo.currbrdnum)
			continue;
		if (uin->invisible && uin->uid != usernum && (!HAS_PERM(PERM_SEECLOAK)))
			continue;
		if (myfriend(uin->uid))
			up->ovrs[up->onum++] = up->num;
		up->users[up->num++] = uin;
	}

	// Extract overriding users
	int n = 0;
	for (i = 0; i < up->num && n < up->onum; ++i) {
		if (up->ovrs[n] == i) {
			if (i != n)
				online_users_swap(up, n, i);
			n++;
		}
	}

	qsort(up->users, up->onum, sizeof(*up->users), comparators[up->sort]);
	qsort(up->users + up->onum, up->num - up->onum, sizeof(*up->users),
			comparators[up->sort]);

	set_num_rows(p);
	return p->all;
}

static tui_list_title_t online_users_title(tui_list_t *p)
{
	online_users_t *up = p->data;
	docmdtitle(up->ovr_only ? "[好朋友列表]" : "[使用者列表]",
			" 聊天[\033[1;32mt\033[m] 寄信[\033[1;32mm\033[m] 送讯息["
			"\033[1;32ms\033[m] 加,减朋友[\033[1;32mo\033[m,\033[1;32md\033[m]"
			" 看说明档[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 切换模式 "
			"[\033[1;32mf\033[m] 求救[\033[1;32mh\033[m]");
	
	const char *field = up->show_note ? "好友说明  " : "使用者昵称";

	char title[256];
	snprintf(title, sizeof(title), "\033[1;44m 编号 %s使用者代号%s %s%s%s    "
			"     %s上站的位置%s     P M %c%s目前动态%s  时:分\033[m\n",
			up->sort == USRSORT_USERID ? "\033[32m{" : " ",
			up->sort == USRSORT_USERID ? "}\033[37m" : " ",
			up->sort == USRSORT_NICK ? "\033[32m{" : " ", field,
			up->sort == USRSORT_NICK ? "}\033[37m" : " ",
			up->sort == USRSORT_FROM ? "\033[32m{" : " ",
			up->sort == USRSORT_FROM ? "}\033[37m" : " ",
			HAS_PERM(PERM_CLOAK) ? 'C' : ' ',
			up->sort == USRSORT_STATUS ? "\033[32m{" : " ",
			up->sort == USRSORT_STATUS ? "}\033[37m" : " ");

	move(2, 0);
	clrtoeol();
	prints("%s", title);
}

static void get_override_note(const char *userid, char *buf, size_t size)
{
	struct override tmp;
	memset(&tmp, 0, sizeof(tmp));

	buf[0] = '\0';
	char file[HOMELEN];
	sethomefile(file, currentuser.userid, "friends");
	if (search_record(buf, &tmp, sizeof(tmp), cmpfnames, (void *)userid))
		strlcpy(buf, tmp.exp, size);
}

extern const char *idle_str(struct user_info *uent);

static tui_list_display_t online_users_display(tui_list_t *p, int i)
{
	online_users_t *up = p->data;

	struct user_info *uin;
	bool is_ovr;
	char buf[STRLEN], line[256];

	is_ovr = up->ovr_only || i < up->onum;
	uin = up->users[i];
	if (!uin || !uin->active || !uin->pid || *uin->userid == '\0') {
		outs("  \033[1;44m啊..我刚走\033[m\n");
		return 0;
	}

	if (is_ovr && up->show_note)
		get_override_note(uin->userid, buf, sizeof(buf));
	else
		strlcpy(buf, uin->username, sizeof(buf));
	ellipsis(buf, 20);

	const char *host;
	if (HAS_PERM2(PERM_OCHAT, &currentuser)) {
		host = uin->from;
	} else {
		if (is_hide_ip(uin))
			host = "......";
		else
			host = mask_host(uin->from);
	}

	char pager;
	if (uin->mode == ST_FIVE || uin->mode == ST_BBSNET
			|| uin->mode == ST_LOCKSCREEN)
		pager = '@';
	else
		pager = pagerchar(hisfriend(uin), uin->pager);

	const char *color;
	if (uin->invisible)
		color = "\033[1;30m";
	else if (is_web_user(uin->mode))
		color = "\033[36m";
	else if (uin->mode == ST_POSTING || uin->mode == ST_MARKET)
		color = "\033[32m";
	else if (uin->mode == ST_FIVE || uin->mode == ST_BBSNET)
		color = "\033[33m";
	else
		color = "";

	snprintf(line, sizeof(line), " \033[m%4d%s%-12.12s\033[37m %-20.20s"
			"\033[m %-15.15s %c %c %c %s%-10.10s\033[37m %5.5s\033[m\n",
			i + 1, is_ovr ? "\033[32m√" : "  ", uin->userid,
			buf, host, pager, msgchar(uin), uin->invisible ? '@' : ' ',
			color, mode_type(uin->mode), idle_str(uin));
	prints("%s", line);
	return 0;
}

static tui_list_query_t online_users_query(tui_list_t *p)
{
	online_users_t *up = p->data;
	struct user_info *uin = up->users[p->cur];
	p->in_query = true;
	if (!uin)
		return DONOTHING;
	t_query(uin->userid);
	move(t_lines - 1, 0);
	prints("\033[0;1;37;44m聊天[\033[1;32mt\033[37m] 寄信[\033[1;32mm\033[37m] "
			"送讯息[\033[1;32ms\033[37m] 加,减朋友[\033[1;32mo\033[37m,\033[1;32md\033[37m] "
			"选择使用者[\033[1;32m↑\033[37m,\033[1;32m↓\033[37m] "
			"求救[\033[1;32mh\033[37m]");
	refresh();
	return DONOTHING;
}

static tui_list_handler_t online_users_handler(tui_list_t *p, int ch)
{
	char buf[STRLEN], tmp[EXT_IDLEN], *ptr;
	online_users_t *up = p->data;
	struct user_info *uin = up->users[p->cur];
	
	p->valid = false;
	switch (ch) {
		case 'h':
		case 'H':
			show_help("help/userlisthelp");
			return FULLUPDATE;
		case 'm':
		case 'M':
			if (!HAS_PERM(PERM_MAIL))
				return DONOTHING;
			m_send(uin->userid);
			return FULLUPDATE;
		case 's':
		case 'S':
			if (!strcmp(currentuser.userid, "guest") || !HAS_PERM(PERM_TALK))
				return DONOTHING;
			if (!canmsg(uin)) {
				snprintf(buf, sizeof(buf), "%s 已关闭讯息呼叫器", uin->userid);
				presskeyfor(buf, t_lines - 1);
				return MINIUPDATE;
			}
			do_sendmsg(uin, NULL, 0, uin->pid);
			return FULLUPDATE;
		case 'o':
		case 'O':
			if (!strcmp(currentuser.userid, "guest"))
				return DONOTHING;
			snprintf(buf, sizeof(buf), "确定关注 %s 吗?", uin->userid);
			if (!askyn(buf, false, true))
				return MINIUPDATE;
			if (follow(session.uid, uin->userid, NULL)) {
				snprintf(buf, sizeof(buf), "成功关注 %s", uin->userid);
				presskeyfor(buf, t_lines - 1);
			}
			return MINIUPDATE;
		case 'd':
		case 'D':
			if (!strcmp(currentuser.userid, "guest"))
				return DONOTHING;
			snprintf(buf, sizeof(buf), "确定不再关注 %s 吗?",
					uin->userid);
			if (!askyn(buf, false, true))
				return MINIUPDATE;
			{
				user_id_t uid = get_user_id(uin->userid);
				if (uid > 0 && unfollow(session.uid, uid)) {
					snprintf(buf, sizeof(buf), "已取消关注 %s", uin->userid);
					presskeyfor(buf, t_lines - 1);
					return PARTUPDATE;
				}
			}		
			return MINIUPDATE;
	}
	if (p->in_query)
		return DONOTHING;

	switch (ch) {
		case 'Y':
			if (HAS_PERM(PERM_CLOAK)) {
				x_cloak();
				up->uptime = 0;
				return PARTUPDATE;
			}
			return DONOTHING;
		case 'P':
			t_pager();
			up->uptime = 0;
			return PARTUPDATE;
		case 'C':
		case 'c':
			if (!strcmp(currentuser.userid, "guest"))
				return DONOTHING;
			buf[0] = '\0';
			if (ch == 'C')
				ptr = "变换昵称(不是临时变换)为: ";
			else
				ptr = "暂时变换昵称(最多10个汉字): ";
			getdata(t_lines - 1, 0, ptr, buf, (ch=='C') ? NAMELEN : 21,
					DOECHO, NA);
			if (buf[0] != '\0') {
				strlcpy(uinfo.username, buf, sizeof(uinfo.username));
				if (ch == 'C') {
					set_safe_record();
					strlcpy(currentuser.username, buf,
							sizeof(currentuser.username));
					substitut_record(PASSFILE, &currentuser,
							sizeof(currentuser), usernum);
				}
				update_ulist(&uinfo, utmpent);
				up->uptime = 0;
				return PARTUPDATE;
			}
			return MINIUPDATE;
		case 'k':
		case 'K':
			if (!HAS_PERM(PERM_USER) && (usernum != uin->uid))
				return DONOTHING;
			if (!strcmp(currentuser.userid, "guest"))
				return DONOTHING;
			if (uin->pid == uinfo.pid)
				strlcpy(buf, "您要把【自己】踢出去吗", sizeof(buf));
			else
				snprintf(buf, sizeof(buf), "你要把 %s 踢出站外吗", uin->userid);
			if (!askyn(buf, false, true))
				return MINIUPDATE;
			strlcpy(tmp, uin->userid, sizeof(tmp));
			if (do_kick_user(uin) == 0) {
				snprintf(buf, sizeof(buf), "%s 已被踢出站外", tmp);
				up->uptime = 0;
				return PARTUPDATE;
			} else {
				snprintf(buf, sizeof(buf), "%s 无法踢出站外", tmp);
				return MINIUPDATE;
			}
		case 'f':
		case 'F':
			up->ovr_only = !up->ovr_only;
			if (up->ovr_only)
				set_user_status(ST_FRIEND);
			else
				set_user_status(ST_LUSERS);
			up->uptime = 0;
			set_num_rows(p);
			return PARTUPDATE;
		case 'W':
		case 'w':
			if (!strcmp(currentuser.userid, "guest"))
				return DONOTHING;
			up->show_note = !up->show_note;
			return PARTUPDATE;
		case KEY_TAB:	
			if (HAS_PERM(PERM_OCHAT)) {
				if (++(up->sort) > USRSORT_STATUS)
					up->sort = USRSORT_USERID;
				up->uptime = 0;
				return FULLUPDATE;
			}
			return DONOTHING;
		case '\r':
		case '\n':
		case KEY_RIGHT:
			online_users_query(p);
			return DONOTHING;
		default:
			return DONOTHING;
	}
}

static int online_users(online_users_t *op)
{
	tui_list_t t = {
		.data = op,
		.loader = online_users_load,
		.title = online_users_title,
		.display = online_users_display,
		.handler = online_users_handler,
		.query = online_users_query,
	};
	
	tui_list(&t);

	free(op->users);
	free(op->ovrs);
	return 0;
}

int online_users_show(void)
{
	online_users_t ou;
	if (online_users_init(&ou) < 0)
		return -1;

	set_user_status(ST_LUSERS);
	return online_users(&ou);
}

int online_users_show_override(void)
{
	set_user_status(ST_FRIEND);

	char file[HOMELEN];
	sethomefile(file, currentuser.userid, "friends");
	if (!dashf(file)) {
		presskeyfor("您尚未设定好友名单，所以...", t_lines - 1);
		return 0;
	}

	online_users_t ou;
	if (online_users_init(&ou) < 0)
		return -1;

	ou.ovr_only = true;

	return online_users(&ou);
}

int online_users_show_board(void)
{
	online_users_t ou;
	if (online_users_init(&ou) < 0)
		return -1;

	ou.board = true;

	set_user_status(ST_LUSERS);
	return online_users(&ou);
}
