# DB Manager index-creation SQL — porting reference

Source: `python/plugins/db_manager/db_plugins/<provider>/connector.py` at
commit `be5e9c637f7^` (the parent of the commit that removed DB Manager).

To re-read later:

```bash
git show be5e9c637f7^:python/plugins/db_manager/db_plugins/postgis/connector.py
# replace "postgis" with gpkg | spatialite | oracle
```

Scope: `createTableIndex`, `deleteTableIndex`, `createSpatialIndex`,
`deleteSpatialIndex`, and `hasSpatialIndex` where it existed.

Note: MSSQL and HANA had no DB Manager connector — no legacy SQL exists for
them. See `src/providers/mssql/qgsmssqlprovider.cpp:1870` for the one
MSSQL attribute-index SQL that lives in-tree today.

---

## PostGIS — `db_plugins/postgis/connector.py:1372`

```python
def createTableIndex(self, table, name, column):
    """Creates index on one column using default options"""
    sql = f"CREATE INDEX {self.quoteId(name)} ON {self.quoteId(table)} ({self.quoteId(column)})"
    self._execute_and_commit(sql)

def deleteTableIndex(self, table, name):
    schema, tablename = self.getSchemaTableName(table)
    sql = "DROP INDEX %s" % self.quoteId((schema, name))
    self._execute_and_commit(sql)

def createSpatialIndex(self, table, geom_column="geom"):
    schema, tablename = self.getSchemaTableName(table)
    idx_name = self.quoteId(f"sidx_{tablename}_{geom_column}")
    sql = f"CREATE INDEX {idx_name} ON {self.quoteId(table)} USING GIST({self.quoteId(geom_column)})"
    self._execute_and_commit(sql)

def deleteSpatialIndex(self, table, geom_column="geom"):
    schema, tablename = self.getSchemaTableName(table)
    idx_name = self.quoteId(f"sidx_{tablename}_{geom_column}")
    return self.deleteTableIndex(table, idx_name)
```

Notes:
- No `UNIQUE` support in DB Manager, but Postgres `CREATE UNIQUE INDEX` works
  — our new API should accept it.
- `createSpatialIndex` is already implemented in-tree at
  `src/providers/postgres/qgspostgresproviderconnection.cpp:661`.

---

## GeoPackage — `db_plugins/gpkg/connector.py:842`

```python
def createTableIndex(self, table, name, column, unique=False):
    """Creates index on one column using default options"""
    unique_str = "UNIQUE" if unique else ""
    sql = f"CREATE {unique_str} INDEX {self.quoteId(name)} ON {self.quoteId(table)} ({self.quoteId(column)})"
    self._execute_and_commit(sql)

def deleteTableIndex(self, table, name):
    schema, tablename = self.getSchemaTableName(table)
    sql = "DROP INDEX %s" % self.quoteId((schema, name))
    self._execute_and_commit(sql)

def createSpatialIndex(self, table, geom_column):
    if self.isRasterTable(table):
        return False
    _, tablename = self.getSchemaTableName(table)
    sql = f"SELECT CreateSpatialIndex({self.quoteId(tablename)}, {self.quoteId(geom_column)})"
    try:
        res = self._fetchOne(sql)
    except QgsProviderConnectionException:
        return False
    return res is not None and res[0][0] == 1

def deleteSpatialIndex(self, table, geom_column):
    if self.isRasterTable(table):
        return False
    _, tablename = self.getSchemaTableName(table)
    sql = f"SELECT DisableSpatialIndex({self.quoteId(tablename)}, {self.quoteId(geom_column)})"
    res = self._fetchOne(sql)
    return len(res) > 0 and len(res[0]) > 0 and res[0][0] == 1

def hasSpatialIndex(self, table, geom_column):
    if self.isRasterTable(table) or geom_column is None:
        return False
    _, tablename = self.getSchemaTableName(table)
    # (only available in >= 2.1.2)
    sql = f"SELECT HasSpatialIndex({self.quoteString(tablename)}, {self.quoteString(geom_column)})"
    gdal.PushErrorHandler()
    ret = self._fetchOne(sql)
    gdal.PopErrorHandler()
    if len(ret) == 0:
        # might be the case for GDAL < 2.1.2
        sql = (
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name LIKE %s"
            % self.quoteString("%%rtree_" + tablename + "_%%")
        )
        ret = self._fetchOne(sql)
    if len(ret) == 0:
        return False
    else:
        return ret[0][0] >= 1
```

Notes:
- GDAL ≥ 2.1.2 fallback branch is almost certainly unneeded in our min-GDAL now.
- Spatial-index CRUD already lives at
  `src/core/providers/ogr/qgsgeopackageproviderconnection.cpp:144`.

---

## SpatiaLite — `db_plugins/spatialite/connector.py:692`

```python
def createTableIndex(self, table, name, column, unique=False):
    """Creates index on one column using default options"""
    unique_str = "UNIQUE" if unique else ""
    sql = f"CREATE {unique_str} INDEX {self.quoteId(name)} ON {self.quoteId(table)} ({self.quoteId(column)})"
    self._execute_and_commit(sql)

def deleteTableIndex(self, table, name):
    schema, tablename = self.getSchemaTableName(table)
    sql = "DROP INDEX %s" % self.quoteId((schema, name))
    self._execute_and_commit(sql)

def createSpatialIndex(self, table, geom_column="geometry"):
    if self.isRasterTable(table):
        return False
    schema, tablename = self.getSchemaTableName(table)
    sql = f"SELECT CreateSpatialIndex({self.quoteString(tablename)}, {self.quoteString(geom_column)})"
    self._execute_and_commit(sql)

def deleteSpatialIndex(self, table, geom_column="geometry"):
    if self.isRasterTable(table):
        return False
    schema, tablename = self.getSchemaTableName(table)
    try:
        sql = f"SELECT DiscardSpatialIndex({self.quoteString(tablename)}, {self.quoteString(geom_column)})"
        self._execute_and_commit(sql)
    except DbError:
        sql = f"SELECT DeleteSpatialIndex({self.quoteString(tablename)}, {self.quoteString(geom_column)})"
        self._execute_and_commit(sql)
        # delete the index table
        idx_table_name = f"idx_{tablename}_{geom_column}"
        self.deleteTable(idx_table_name)

def hasSpatialIndex(self, table, geom_column="geometry"):
    if not self.has_geometry_columns or self.isRasterTable(table):
        return False
    c = self._get_cursor()
    schema, tablename = self.getSchemaTableName(table)
    sql = f"SELECT spatial_index_enabled FROM geometry_columns WHERE upper(f_table_name) = upper({self.quoteString(tablename)}) AND upper(f_geometry_column) = upper({self.quoteString(geom_column)})"
    self._execute(c, sql)
    row = c.fetchone()
    return row is not None and row[0] == 1
```

Notes:
- Spatial-index CRUD already lives at
  `src/providers/spatialite/qgsspatialiteproviderconnection.cpp:255`.
- `deleteSpatialIndex` fallback (`DiscardSpatialIndex` → `DeleteSpatialIndex`
  + drop the `idx_…` virtual table) is a quirk worth copying into the C++
  override if we add `deleteIndex` for spatial columns.

---

## Oracle — `db_plugins/oracle/connector.py:1591`

```python
def createTableIndex(self, table, name, column):
    """Creates index on one column using default options."""
    sql = f"CREATE INDEX {self.quoteId(name)} ON {self.quoteId(table)} ({self.quoteId(column)})"
    self._execute_and_commit(sql)

def rebuildTableIndex(self, table, name):
    """Rebuilds a table index"""
    schema, tablename = self.getSchemaTableName(table)
    sql = f"ALTER INDEX {self.quoteId((schema, name))} REBUILD"
    self._execute_and_commit(sql)

def deleteTableIndex(self, table, name):
    """Deletes an index on a table."""
    schema, tablename = self.getSchemaTableName(table)
    sql = f"DROP INDEX {self.quoteId((schema, name))}"
    self._execute_and_commit(sql)

def createSpatialIndex(self, table, geom_column="GEOM"):
    """Creates a spatial index on a geometric column."""
    geom_column = geom_column.upper()
    schema, tablename = self.getSchemaTableName(table)
    idx_name = self.quoteId(f"sidx_{tablename}_{geom_column}")
    sql = f"""
    CREATE INDEX {idx_name}
    ON {self.quoteId(table)}({self.quoteId(geom_column)})
    INDEXTYPE IS MDSYS.SPATIAL_INDEX
    """
    self._execute_and_commit(sql)

def deleteSpatialIndex(self, table, geom_column="GEOM"):
    """Deletes a spatial index of a geometric column."""
    schema, tablename = self.getSchemaTableName(table)
    idx_name = self.quoteId(f"sidx_{tablename}_{geom_column}")
    return self.deleteTableIndex(table, idx_name)
```

Notes:
- Oracle also had a `rebuildTableIndex` DB Manager uniquely exposed —
  out of scope for the context-menu port but noted.
- Spatial-index CRUD already lives at
  `src/providers/oracle/qgsoracleproviderconnection.cpp:1533`.
- Reminder: Oracle browser items don't currently expose `QgsFieldsItem`,
  so the context-menu entry can't fire there until that's added.

---

## When this file has served its purpose

Delete it. It is not meant to ship; it only exists to shortcut porting work
while `src/core/providers/qgsabstractdatabaseproviderconnection.{h,cpp}` and
the per-provider connection classes are being extended with `createIndex`
(and optionally `indexExists` / `deleteIndex`).
