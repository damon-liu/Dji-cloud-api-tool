import { describe, expect, it } from 'vitest'
import { getByPath, parseMappedFields } from '../services/topicMapping'
import type { TopicMapping } from '../types/domain'

const mapping: TopicMapping = {
  topics: {
    'thing/product/{sn}/osd': {
      description: 'OSD',
      fields: {
        'battery.capacity_percent': { zh: '电池电量', unit: '%' },
        mode_code: { zh: '模式码', values: { '0': '待机', '9': '自动返航' } },
      },
      groups: [{ id: 'basic', label: '基础信息', keys: ['mode_code', 'battery.capacity_percent'] }],
    },
  },
}

const arrayMapping: TopicMapping = {
  topics: {
    'thing/product/{sn}/osd': {
      description: 'OSD',
      fields: {
        'dongle_infos[].imei': { zh: '增强图传 IMEI' },
      },
      groups: [{ id: 'basic', label: '基础信息', keys: ['dongle_infos[].imei'] }],
    },
  },
}

describe('topicMapping', () => {
  it('reads nested values by dot path', () => {
    expect(getByPath({ battery: { capacity_percent: 90 } }, 'battery.capacity_percent')).toBe(90)
  })

  it('reads values with numeric array selectors', () => {
    expect(getByPath(
      { battery: { batteries: [{ temperature: 31 }] } },
      'battery.batteries[0].temperature',
    )).toBe(31)
  })

  it('collects values with wildcard array selectors', () => {
    expect(getByPath(
      { dongle_infos: [{ imei: 'a' }, { imei: 'b' }] },
      'dongle_infos[].imei',
    )).toEqual(['a', 'b'])
    expect(getByPath(
      { sub_devices: [{ sn: 's1' }, { sn: 's2' }] },
      'sub_devices[].sn',
    )).toEqual(['s1', 's2'])
  })

  it('maps fields with zh labels units and enum values', () => {
    const rows = parseMappedFields(mapping, 'thing/product/dock_001/osd', {
      mode_code: 9,
      battery: { capacity_percent: 90 },
    })

    expect(rows).toEqual([
      { key: 'mode_code', label: '模式码', value: '自动返航', rawValue: 9, unit: '', groupId: 'basic' },
      { key: 'battery.capacity_percent', label: '电池电量', value: '90', rawValue: 90, unit: '%', groupId: 'basic' },
    ])
  })

  it('displays wildcard array values as a joined string', () => {
    const rows = parseMappedFields(arrayMapping, 'thing/product/dock_001/osd', {
      dongle_infos: [{ imei: 'a' }, { imei: 'b' }],
    })

    expect(rows).toEqual([
      {
        key: 'dongle_infos[].imei',
        label: '增强图传 IMEI',
        value: 'a, b',
        rawValue: ['a', 'b'],
        unit: '',
        groupId: 'basic',
      },
    ])
  })
})
