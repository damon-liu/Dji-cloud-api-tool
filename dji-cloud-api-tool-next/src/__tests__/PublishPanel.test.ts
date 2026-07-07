import { beforeEach, describe, expect, it, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import PublishPanel from '../components/PublishPanel.vue'
import { useDeviceStore } from '../stores/deviceStore'
import { useTopicStore } from '../stores/topicStore'
import { tauriApi } from '../services/tauriApi'

vi.mock('../services/tauriApi', () => ({
  tauriApi: {
    publishMessage: vi.fn(),
    saveTopics: vi.fn(),
  },
}))

const publishMessage = vi.mocked(tauriApi.publishMessage)
const saveTopics = vi.mocked(tauriApi.saveTopics)

describe('PublishPanel', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
    document.body.innerHTML = ''
    saveTopics.mockResolvedValue()
  })

  it('blocks invalid JSON payloads before publishing', async () => {
    const topics = useTopicStore()
    await topics.addTopic('dock_001', 'thing/product/dock_001/osd')
    mountPanel()

    await setPayload('{ bad json')
    await clickPublish()

    expect(publishMessage).not.toHaveBeenCalled()
    expect(document.body.textContent).toContain('JSON 格式错误:')
  })

  it('does not render the publish form without a selected device', async () => {
    mount(PublishPanel, {
      attachTo: document.body,
    })

    expect(document.body.textContent).toContain('请选择设备')
    expect(document.body.querySelector('[data-test="publish-send"]')).toBeNull()
  })

  it('publishes custom topic over the selected device topic after JSON validation', async () => {
    publishMessage.mockResolvedValue()
    const topics = useTopicStore()
    await topics.addTopic('dock_001', 'thing/product/dock_001/osd')
    await topics.addTopic('dock_001', 'thing/product/dock_001/services')
    topics.selectTopic('dock_001', 'thing/product/dock_001/services')
    mountPanel()

    await setCustomTopic('thing/product/dock_001/custom')
    await setPayload('{"method":"ping"}')
    await clickPublish()

    expect(publishMessage).toHaveBeenCalledWith('thing/product/dock_001/custom', '{"method":"ping"}')
    expect(document.body.textContent).toContain('下发成功')
  })

  it('shows string errors returned by the publish command', async () => {
    publishMessage.mockRejectedValue('not connected')
    const topics = useTopicStore()
    await topics.addTopic('dock_001', 'thing/product/dock_001/osd')
    mountPanel()

    await clickPublish()

    expect(document.body.textContent).toContain('not connected')
  })
})

function mountPanel() {
  const devices = useDeviceStore()
  devices.devices = [{ sn: 'dock_001', name: 'Dock 001', type: 'dock', online: false }]
  devices.selectedSn = 'dock_001'

  return mount(PublishPanel, {
    attachTo: document.body,
  })
}

async function setPayload(value: string) {
  const textarea = document.body.querySelector<HTMLTextAreaElement>('[data-test="publish-payload"]')
  expect(textarea).not.toBeNull()
  textarea!.value = value
  textarea!.dispatchEvent(new Event('input'))
  await Promise.resolve()
}

async function setCustomTopic(value: string) {
  const input = document.body.querySelector<HTMLInputElement>('[data-test="publish-custom-topic"]')
  expect(input).not.toBeNull()
  input!.value = value
  input!.dispatchEvent(new Event('input'))
  await Promise.resolve()
}

async function clickPublish() {
  const button = document.body.querySelector<HTMLButtonElement>('[data-test="publish-send"]')
  expect(button).not.toBeNull()
  button!.click()
  await Promise.resolve()
  await Promise.resolve()
}
