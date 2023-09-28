CREATE TYPE md5hash;

-- input/output function
CREATE FUNCTION md5_in (cstring)
  RETURNS md5hash
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_out (md5hash)
  RETURNS cstring
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

-- functions backing operators (and btree opclass)
CREATE FUNCTION md5_eq (md5hash, md5hash)
  RETURNS boolean
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_neq (md5hash, md5hash)
  RETURNS boolean
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_leq (md5hash, md5hash)
  RETURNS boolean
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_lt (md5hash, md5hash)
  RETURNS boolean
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_geq (md5hash, md5hash)
  RETURNS boolean
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_gt (md5hash, md5hash)
  RETURNS boolean
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_cmp (md5hash, md5hash)
  RETURNS int
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_recv (internal)
  RETURNS md5hash
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_send (md5hash)
  RETURNS bytea
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE FUNCTION md5_bytea_in (bytea)
  RETURNS md5hash
  AS 'md5hash'
  LANGUAGE C
  IMMUTABLE STRICT;

CREATE TYPE md5hash (
  INPUT = md5_in, OUTPUT = md5_out, INTERNALLENGTH = 16, SEND = md5_send, RECEIVE = md5_recv
);

-- some basic support for implicit casts
CREATE CAST (md5hash AS text) WITH INOUT AS IMPLICIT;

CREATE CAST (text AS md5hash) WITH INOUT AS IMPLICIT;

CREATE CAST (bytea AS md5hash) WITH FUNCTION md5_bytea_in (bytea) AS IMPLICIT;

CREATE CAST (md5hash AS bytea) WITH FUNCTION md5_send (md5hash) AS IMPLICIT;

CREATE OPERATOR = (
  PROCEDURE = md5_eq, LEFTARG = md5hash, RIGHTARG = md5hash, COMMUTATOR = =, NEGATOR = <>, RESTRICT = eqsel, JOIN = eqjoinsel, MERGES, HASHES
);

CREATE OPERATOR <> (
  PROCEDURE = md5_neq, LEFTARG = md5hash, RIGHTARG = md5hash, COMMUTATOR = <>, NEGATOR = =, RESTRICT = neqsel, JOIN = neqjoinsel
);

CREATE OPERATOR < (
  PROCEDURE = md5_lt, LEFTARG = md5hash, RIGHTARG = md5hash, COMMUTATOR = >, NEGATOR = >=, RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE OPERATOR > (
  PROCEDURE = md5_gt, LEFTARG = md5hash, RIGHTARG = md5hash, COMMUTATOR = <, NEGATOR = <=, RESTRICT = scalargtsel, JOIN = scalargtjoinsel
);

CREATE OPERATOR <= (
  PROCEDURE = md5_leq, LEFTARG = md5hash, RIGHTARG = md5hash, COMMUTATOR = >=, NEGATOR = >, RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE OPERATOR >= (
  PROCEDURE = md5_geq, LEFTARG = md5hash, RIGHTARG = md5hash, COMMUTATOR = <=, NEGATOR = <, RESTRICT = scalargtsel, JOIN = scalargtjoinsel
);

CREATE OPERATOR CLASS btree_md5_ops DEFAULT FOR TYPE md5hash
  USING btree AS
  OPERATOR 1 <, OPERATOR 2 <=, OPERATOR 3 =, OPERATOR 4 >=, OPERATOR 5 >, FUNCTION 1 md5_cmp ( md5hash, md5hash
);
