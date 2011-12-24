BEGIN;

CREATE TABLE all_users (
	id SERIAL PRIMARY KEY,
	name TEXT,
	passwd TEXT,
	alive BOOLEAN DEFAULT TRUE,
	money BIGINT DEFAULT 0,
	rank REAL DEFAULT 0,
	paid_posts INTEGER DEFAULT 0,
	title TEXT,
);
CREATE OR REPLACE VIEW users AS
	SELECT * FROM all_users WHERE alive = TRUE;
CREATE UNIQUE INDEX user_name_idx ON all_users (lower(name)) WHERE alive = TRUE;

CREATE TABLE board_sectors (
	id SERIAL PRIMARY KEY,
	name CHAR,
	descr TEXT,
	short_descr TEXT
);
CREATE TABLE board_categs (
	id SERIAL PRIMARY KEY,
	name TEXT
);
CREATE TABLE boards (
	id SERIAL PRIMARY KEY,
	name TEXT,
	descr TEXT,
	parent INTEGER,
	flag INTEGER,
	perm INTEGER,
	categ INTEGER REFERENCES board_categs,
	sector INTEGER REFERENCES board_sectors,
	bms TEXT
);
CREATE UNIQUE INDEX ON boards (lower(bms));
CREATE TABLE bms (
	user_id INTEGER REFERENCES all_users,
	board_id INTEGER REFERENCES boards,
	UNIQUE (user_id, board_id)
);

CREATE TABLE shopping_categories (
	id SERIAL PRIMARY KEY,
	name TEXT
);
CREATE TABLE shopping_items (
	id SERIAL PRIMARY KEY,
	category INTEGER REFERENCES shopping_categories,
	name TEXT,
	price INTEGER
);
CREATE TABLE shopping_records (
	id SERIAL PRIMARY KEY,
	user_id REFERENCES all_users,
	item INTEGER REFERENCES shopping_items,
	price TEXT,
	order_time TIMESTAMPTZ
);

CREATE TABLE titles (
	id SERIAL PRIMARY KEY,
	user_id INTEGER REFERENCES all_users,
	granter INTEGER REFERENCES all_users,
	title TEXT NOT NULL,
	add_time TIMESTAMPTZ,
	expire TIMESTAMPTZ,
	approved BOOLEAN DEFAULT FALSE,
	paid INTEGER
);

COMMIT;
