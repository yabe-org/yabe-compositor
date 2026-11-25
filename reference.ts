import data from '../data/yabe.reference.json';

export type Type = {
  kind: 'nil' | 'boolean' | 'integer' | 'number' | 'string'
      | 'dispatcher' | 'array' | 'function' | 'table';
  label: string;
  element?: Type;
  parameters?: Parameter[];
  table?: string;
};
export type Parameter = { name: string; type: Type };
export type Fn = {
  name: string; path: string; signature: string; description: string;
  parameters: Parameter[]; returns: Type;
};
export type Namespace = {
  path: string; functions: Fn[]; namespaces: string[];
  constants: { name: string; path: string; type: Type }[];
};
export type Field = {
  name: string; keyPath: string; type: Type; description: string;
  default?: boolean | number | string;
};
export type Table = { name: string; keyPath: string; fields: Field[] };
export type Reference = {
  namespaces: Record<string, Namespace>;
  tables: Record<string, Table>;
};

export const reference = data as Reference;

export function getNamespace(path: string): Namespace {
  const ns = reference.namespaces[path];
  if (!ns) throw new Error(`unknown namespace: ${path}`);
  return ns;
}
export function getTable(name: string): Table {
  const table = reference.tables[name];
  if (!table) throw new Error(`unknown table: ${name}`);
  return table;
}

export const configAnchor = (keyPath: string) => `config-${keyPath}`;
