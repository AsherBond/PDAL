(readers.pgpointcloud)=

# readers.pgpointcloud

The **PostgreSQL Pointcloud Reader** allows you to read points from a PostgreSQL
database with [PostgreSQL Pointcloud] extension enabled. The Pointcloud
extension stores point cloud data in tables that contain rows of patches. Each
patch in turn contains a large number of spatially nearby points.

The reader pulls patches from a table, potentially sub-setting the query
with a "where" clause.

```{eval-rst}
.. plugin::
```

## Example

```json
[
    {
        "type":"readers.pgpointcloud",
        "connection":"dbname='lidar' user='user'",
        "table":"lidar",
        "column":"pa",
        "spatialreference":"EPSG:26910",
        "where":"PC_Intersects(pa, ST_MakeEnvelope(560037.36, 5114846.45, 562667.31, 5118943.24, 26910))"
    },
    {
        "type":"writers.text",
        "filename":"output.txt"
    }
]
```

## Options

```{include} reader_opts.md
```

connection

: PostgreSQL connection string. In the form *"host=hostname dbname=database user=username password=pw port=5432"* \[Required\]

table

: Database table to read from. \[Required\]

schema

: Database schema to read from. \[Default: **public**\]

column

: Table column to read patches from. \[Default: **pa**\]

[postgresql pointcloud]: https://github.com/pramsey/pointcloud
