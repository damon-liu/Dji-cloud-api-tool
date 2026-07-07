import type { TopicMapping } from '../types/domain'

export type ParsedFieldRow = {
  key: string
  label: string
  value: string
  rawValue: unknown
  unit: string
  groupId: string
}

export function getByPath(source: unknown, path: string): unknown {
  if (!path) return undefined

  return path.split('.').reduce<unknown>((current, part) => {
    if (current === null || typeof current !== 'object' || !part) {
      return undefined
    }

    return Object.prototype.hasOwnProperty.call(current, part)
      ? (current as Record<string, unknown>)[part]
      : undefined
  }, source)
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

export function matchTopicTemplate(mapping: TopicMapping, topic: string): string | undefined {
  return Object.keys(mapping.topics).find((template) => templateToRegex(template).test(topic))
}

export function parseMappedFields(
  mapping: TopicMapping,
  topic: string,
  data: Record<string, unknown>,
): ParsedFieldRow[] {
  const template = matchTopicTemplate(mapping, topic)
  if (!template) return []

  const entry = mapping.topics[template]

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
        value: field.values?.[valueKey] ?? valueKey,
        rawValue,
        unit: field.unit ?? '',
        groupId: group.id,
      }]
    }),
  )
}
