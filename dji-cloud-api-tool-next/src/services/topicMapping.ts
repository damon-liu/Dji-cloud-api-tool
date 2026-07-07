import type { TopicMapping } from '../types/domain'
import defaultTopicMappings from '../assets/default-topic-mappings.json'

export const defaultTopicMapping = defaultTopicMappings as TopicMapping

export type ParsedFieldRow = {
  key: string
  label: string
  value: string
  rawValue: unknown
  unit: string
  groupId: string
}

type PathToken = {
  property: string
  selector?: number | 'all'
}

function parsePathToken(part: string): PathToken | undefined {
  const match = part.match(/^([^\[\]]+)(?:\[(\d*)\])?$/)
  if (!match) return undefined

  const [, property, selector] = match
  if (selector === undefined) return { property }
  if (selector === '') return { property, selector: 'all' }

  return { property, selector: Number(selector) }
}

function isPathToken(token: PathToken | undefined): token is PathToken {
  return token !== undefined
}

function readProperty(source: unknown, property: string): unknown {
  if (source === null || typeof source !== 'object') {
    return undefined
  }

  return Object.prototype.hasOwnProperty.call(source, property)
    ? (source as Record<string, unknown>)[property]
    : undefined
}

function resolvePath(source: unknown, tokens: PathToken[]): unknown {
  if (tokens.length === 0) return source

  const [token, ...remainingTokens] = tokens
  const value = readProperty(source, token.property)

  if (token.selector === undefined) {
    return resolvePath(value, remainingTokens)
  }

  if (!Array.isArray(value)) {
    return undefined
  }

  if (token.selector === 'all') {
    const values = value
      .map((item) => resolvePath(item, remainingTokens))
      .filter((item) => item !== undefined)

    return values.length > 0 ? values : undefined
  }

  return resolvePath(value[token.selector], remainingTokens)
}

export function getByPath(source: unknown, path: string): unknown {
  if (!path) return undefined

  const tokens = path.split('.').map(parsePathToken)
  if (!tokens.every(isPathToken)) return undefined

  return resolvePath(source, tokens)
}

function escapeRegexSegment(segment: string): string {
  return segment.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
}

function templateToRegex(template: string): RegExp {
  const pattern = template
    .split('/')
    .map((segment) => segment === '{sn}' ? '[^/]+' : escapeRegexSegment(segment))
    .join('/')

  return new RegExp(`^${pattern}$`)
}

function formatValue(rawValue: unknown): string {
  return Array.isArray(rawValue)
    ? rawValue.map((item) => String(item)).join(', ')
    : String(rawValue)
}

export function matchTopicTemplate(mapping: TopicMapping, topic: string): string | undefined {
  return Object.keys(mapping.topics).find((template) => templateToRegex(template).test(topic))
}

export function mappingForTopic(mapping: TopicMapping, topic: string): TopicMapping['topics'][string] | undefined {
  const template = matchTopicTemplate(mapping, topic)
  return template ? mapping.topics[template] : undefined
}

export function parseMappedFields(
  mapping: TopicMapping,
  topic: string,
  data: Record<string, unknown>,
): ParsedFieldRow[] {
  const entry = mappingForTopic(mapping, topic)
  if (!entry) return []

  return entry.groups.flatMap((group) =>
    group.keys.flatMap((key) => {
      const field = entry.fields[key]
      if (!field) return []

      const rawValue = getByPath(data, key)
      if (rawValue === undefined) return []

      const valueKey = String(rawValue)

      return [{
        key,
        label: field.zh,
        value: field.values?.[valueKey] ?? formatValue(rawValue),
        rawValue,
        unit: field.unit ?? '',
        groupId: group.id,
      }]
    }),
  )
}
