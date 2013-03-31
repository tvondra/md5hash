CREATE EXTENSION md5hash;

SELECT md5('A')::md5hash;
SELECT md5('B')::md5hash;
SELECT md5('A')::md5hash = md5('A')::md5hash;
SELECT md5('A')::md5hash = md5('B')::md5hash;

SELECT md5('A')::md5hash <= md5('B')::md5hash;
SELECT md5('A')::md5hash <  md5('B')::md5hash;
SELECT md5('A')::md5hash >= md5('B')::md5hash;
SELECT md5('A')::md5hash >  md5('B')::md5hash;

CREATE TABLE test ( id md5hash );
INSERT INTO test SELECT md5(i::text) FROM generate_series(1,10000) s(i);
ANALYZE test;
SELECT relpages FROM pg_class WHERE relname = 'test';