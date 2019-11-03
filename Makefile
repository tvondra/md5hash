MODULE_big = md5hash
OBJS = md5hash.o

EXTENSION = md5hash
DATA = sql/md5hash--1.0.1.sql sql/md5hash--1.0.0--1.0.1.sql
MODULES = md5hash

CFLAGS=`pg_config --includedir-server`

TESTS        = $(wildcard test/sql/*.sql)
REGRESS      = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
