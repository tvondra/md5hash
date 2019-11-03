md5hash data type
=================
This extension provides a simple data type storing 128-bit values
(e.g. MD5 hashes) in a bit more efficient way - in a fixed-length
columns and inline.

The extension defines the 'md5hash' type itself, casts, operators
and an operator class for btree indexes. It's fairly straightforward
and simple to add more operators or classes.


Installation
------------
Installing this is very simple, especially if you're using pgxn client.
All you need to do is this:

    $ pgxn install md5hash
    $ pgxn load -d mydb md5hash

and you're done. You may also install the extension manually:

    $ make install
    $ psql dbname -c "CREATE EXTENSION md5hash"

And if you're on an older version (pre-9.1), you have to run the SQL
script manually

    $ psql dbname < `pg_config --sharedir`/contrib/md5hash--1.0.1.sql

That's all you need to do.


Usage
-----
Basically all you need to do is use the 'md5hash' just like any other
data type in CREATE TABLE statement

    CREATE TABLE test_table (
       id md5hash PRIMARY KEY,
       ...
    );

Notice this already creates an index on the md5hash column. Then you may
start querying the column just like any other data type:

    SELECT * FROM test_table
     WHERE id = 'c4ca4238a0b923820dcc509a6f75849b'

And so on. Nothing extraordinary.


Benefits
--------
Let's do some very simple benchmarking, demonstrating the benefits of
this data type.

First let's look at space required to store MD5 hashes in the plain TEXT
column and in the md5hash data type. Try for example this:

    CREATE TABLE test_md5hash (id md5hash       PRIMARY KEY);
    CREATE TABLE test_text    (id varchar(32)   PRIMARY KEY);
    
    INSERT INTO test_md5hash SELECT md5(i::text)
                               FROM generate_series(1,1000000) s(i);
    
    INSERT INTO test_text SELECT md5(i::text)
                               FROM generate_series(1,1000000) s(i);

    SELECT relname,
        (pg_relation_size(oid)/1024) AS relation_size_kB,
        (pg_total_relation_size(oid)/1024) AS total_size_kB
      FROM pg_class WHERE relname LIKE 'test_%';

          relname      | relation_size_kb | total_size_kb 
    -------------------+------------------+---------------
     test_md5hash      |            43248 |         82744
     test_md5hash_pkey |            39464 |         39464
     test_text         |            66672 |        141816
     test_text_pkey    |            75096 |         75096
    (4 rows)

Yup, the md5hash type requires only about 60% space compared to the TEXT
data type. Not bad, especially on systems that are low on memory (which
is a rather common situation). This is because md5hash data type stores
the data in a binary format in 16B, while TEXT stores the data as text,
i.e. 32B (16B in hex) and a varlena header (at least 1B).

The difference is smaller because each tuple has a header (24B or more).
And obviously if there are more columns in the table, the difference will
be even smaller. However with indexes, the difference will remain.

Now, let's see the difference when querying the column - to eliminate
planning overhead, I've used prepared statements and execute 1.000.000
queries with 'WHERE id = $1' condition (using the PK index).

    TEXT     129 seconds
    md5hash  117 seconds

Not a terrible improvement, but still interesting. And the difference
will grow as the memory gets stressed.


License
-------
This software is distributed under the terms of BSD 2-clause license.
See LICENSE or http://www.opensource.org/licenses/bsd-license.php for
more details.
