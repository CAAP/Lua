#!/usr/local/bin/lua

local sql = require'carlos.sqlite'

local conn = sql.connect'/db/inventario.db'

assert(conn.exec'ATTACH DATABASE "/db/ferre.db" AS FR')

assert(conn.exec'ATTACH DATABASE "/db/prov.db" AS PR')

conn.exec'CREATE TABLE IF NOT EXISTS inventario (clave PRIMARY KEY, faltante INTEGER DEFAULT 0, obs DEFAULT "", gps, proveedor)'

conn.exec'INSERT INTO inventario SELECT faltantes.clave, faltante, obs, "" gps, proveedor FROM faltantes LEFT OUTER JOIN proveedores ON faltantes.clave == proveedores.clave'

conn.exec"UPDATE inventario SET proveedor = '' WHERE proveedor LIKE ' %'"
