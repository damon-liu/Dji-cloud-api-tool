<template>
  <div class="shell">
    <header class="toolbar">
      <button type="button">配置</button>
      <span class="toolbar-spacer" />
      <span class="broker-label">未连接 broker</span>
      <button type="button" class="primary">连接</button>
      <button type="button">断开</button>
    </header>

    <main class="workspace">
      <aside class="left-column">
        <DeviceTree />
        <TopicList />
      </aside>

      <section class="right-column">
        <div class="monitor-grid">
          <div class="analysis-column">
            <OsdPanel />
            <ParsedJsonPanel />
          </div>
          <RawJsonPanel />
        </div>
        <PublishPanel />
      </section>
    </main>

    <footer class="statusbar">
      <span>未连接</span>
      <span>DJI Cloud API Tool Next</span>
      <span>设备: 0</span>
    </footer>
  </div>
</template>

<script setup lang="ts">
import DeviceTree from './DeviceTree.vue'
import OsdPanel from './OsdPanel.vue'
import ParsedJsonPanel from './ParsedJsonPanel.vue'
import PublishPanel from './PublishPanel.vue'
import RawJsonPanel from './RawJsonPanel.vue'
import TopicList from './TopicList.vue'
</script>
