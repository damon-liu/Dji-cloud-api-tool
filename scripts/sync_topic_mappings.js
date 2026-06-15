#!/usr/bin/env node
/**
 * 同步 dock-osd.md 到 topic_mappings.json，自动补充缺失字段映射。
 *
 * 用法:
 *   node scripts/sync_topic_mappings.js          # 执行同步
 *   node scripts/sync_topic_mappings.js --dry-run # 仅报告差异
 */

const fs = require('fs');
const path = require('path');

// ============================================================
// 常量
// ============================================================

const PROJECT_ROOT = path.resolve(__dirname, '..');
const MD_PATH = path.join(PROJECT_ROOT, 'config', 'dock-osd.md');
const JSON_PATH = path.join(PROJECT_ROOT, 'config', 'topic_mappings.json');
const TOPIC_KEY = 'thing/product/{sn}/osd';

// ============================================================
// 分组规则: [keyPrefix, groupId, groupLabel]
// 按顺序匹配，命中第一个规则后停止
// ============================================================
const GROUP_RULES = [
    ["air_conditioner.",           "air_conditioner", "❄️ 空调"],
    ["rtcm_info.",                 "rtcm",            "📡 RTK标定"],
    ["dongle_infos[].",            "dongle",          "📶 4G Dongle"],
    ["wireless_link.",             "wireless",        "📶 图传链路"],
    ["wireless_link_topo.",        "wireless",        "📶 图传链路"],
    ["live_capacity.",             "live",            "🎥 直播"],
    ["live_status[].",             "live",            "🎥 直播"],
    ["maintain_status.",           "maintain",        "🔧 保养"],
    ["sub_device.",                "sub_device",      "📱 子设备"],
    ["media_file_detail.",         "media",           "📁 媒体"],
    ["drone_battery_maintenance_info.heat_state",  "battery_ext", "🔋 电池扩展"],
    ["drone_battery_maintenance_info.batteries[].", "battery_ext", "🔋 电池扩展"],
    ["position_state.is_calibration", "position_ext", "📍 定位扩展"],
    ["home_position_is_valid",     "position_ext",    "📍 定位扩展"],
    ["heading",                    "position_ext",    "📍 定位扩展"],
    ["self_converge_coordinate.",  "position_ext",    "📍 定位扩展"],
    ["drc_state",                  "position_ext",    "📍 定位扩展"],
];

const FALLBACK_GROUP_ID = "device_ext";
const FALLBACK_GROUP_LABEL = "🔧 设备扩展";

// ============================================================
// Markdown 解析
// ============================================================

/**
 * 统计字符串开头的 » 数量
 */
function countPrefix(s) {
    const content = s.trim();
    let count = 0;
    for (const ch of content) {
        if (ch === '»') {  // »
            count++;
        } else {
            break;
        }
    }
    return count;
}

/**
 * 去除开头的 » 及空格，返回干净字段名
 */
function stripPrefix(s) {
    return s.trim().replace(/^[»\s]+/u, '');
}

/**
 * 解析 constraint 列 JSON，返回 { values, unit }
 */
function parseConstraint(raw) {
    const values = {};
    let unit = "";

    const trimmed = raw.trim();
    if (!trimmed) return { values, unit };

    let obj;
    try {
        obj = JSON.parse(trimmed);
    } catch (e) {
        console.warn(`  ⚠ 警告: constraint JSON 解析失败: ${trimmed.slice(0, 80)}`);
        return { values, unit };
    }

    if (typeof obj !== 'object' || obj === null) return { values, unit };

    // 提取 unit: 取 unit_name 中 / 后的简短形式
    if (typeof obj.unit_name === 'string' && obj.unit_name) {
        if (obj.unit_name.includes(' / ')) {
            unit = obj.unit_name.split(' / ')[1];
        } else {
            unit = obj.unit_name;
        }
    }

    // 提取枚举值: 过滤掉 meta 键
    const metaKeys = new Set(["max", "min", "step", "unit_name", "item_type", "size", "length", "desc"]);
    for (const [k, v] of Object.entries(obj)) {
        if (!metaKeys.has(k) && typeof v === 'string') {
            values[k] = v;
        }
    }

    return { values, unit };
}

/**
 * 解析 dock-osd.md 表格，返回叶子字段列表
 */
function parseDockOsdMd(mdPath) {
    if (!fs.existsSync(mdPath)) {
        console.error(`错误: 文件不存在: ${mdPath}`);
        process.exit(1);
    }

    const fields = [];
    const depthStack = [];  // [{ prefix, isArray }]

    const lines = fs.readFileSync(mdPath, 'utf-8').split(/\r?\n/);
    const tableRowRe = /^\| .+ \|$/;
    const separatorRe = /^\|[\s\-:]+\|/;
    const headerRe = /^\| Column/;

    for (const line of lines) {
        if (!tableRowRe.test(line)) continue;
        if (separatorRe.test(line)) continue;
        if (headerRe.test(line)) continue;  // 跳过表头行

        // 分割单元格
        const cols = line.split('|').slice(1, -1).map(c => c.trim());
        if (cols.length < 5) continue;

        const keyRaw = cols[0];
        const nameCn = cols[1];
        const typeStr = cols[2];
        const constraintRaw = cols[3];
        const desc = cols.length > 4 ? cols[4] : "";

        // 计算深度
        const depth = countPrefix(keyRaw);
        const fieldName = stripPrefix(keyRaw);
        const isArrayType = (typeStr === 'array');

        // 调整深度栈
        while (depthStack.length > depth) {
            depthStack.pop();
        }

        if (depth === 0) {
            // 新顶层节点
            depthStack.length = 0;
            if (isArrayType || typeStr === 'struct') {
                depthStack.push({ prefix: fieldName, isArray: isArrayType });
            } else {
                const { values, unit } = parseConstraint(constraintRaw);
                fields.push({ key: fieldName, zh: nameCn, type: typeStr, values, unit, description: desc });
            }
        } else {
            // 子字段
            const parent = depthStack[depthStack.length - 1];
            const fullKey = parent.isArray
                ? `${parent.prefix}[].${fieldName}`
                : `${parent.prefix}.${fieldName}`;

            if (isArrayType || typeStr === 'struct') {
                depthStack.push({ prefix: fullKey, isArray: isArrayType });
            } else {
                const { values, unit } = parseConstraint(constraintRaw);
                fields.push({ key: fullKey, zh: nameCn, type: typeStr, values, unit, description: desc });
            }
        }
    }

    return fields;
}

// ============================================================
// 字段合并
// ============================================================

/**
 * 按 group_id 对字段 key 分组
 */
function buildZones(fields) {
    const groups = {};
    for (const f of fields) {
        let groupId = FALLBACK_GROUP_ID;
        for (const [prefix, gid] of GROUP_RULES) {
            if (f.key.startsWith(prefix)) {
                groupId = gid;
                break;
            }
        }
        if (!groups[groupId]) groups[groupId] = [];
        groups[groupId].push(f.key);
    }
    return groups;
}

/**
 * 合并 dock 字段到现有 topic_mappings
 */
function mergeFields(dockFields, existingMappings) {
    const topics = existingMappings.topics || {};
    let topic = topics[TOPIC_KEY];

    if (!topic) {
        topic = {
            description: "机场/无人机 OSD 遥测数据 (0.5Hz 定时上报)",
            fields: {},
            groups: []
        };
    }

    const existingFields = topic.fields || {};
    const existingGroups = topic.groups || [];
    const existingKeys = new Set(Object.keys(existingFields));
    const existingGroupIds = new Set(existingGroups.map(g => g.id));

    let addedCount = 0;
    let skippedCount = 0;

    // 添加缺失字段
    for (const f of dockFields) {
        if (existingKeys.has(f.key)) {
            skippedCount++;
            continue;
        }

        const fieldDef = { zh: f.zh, unit: f.unit };
        if (Object.keys(f.values).length > 0) {
            fieldDef.values = f.values;
        }

        existingFields[f.key] = fieldDef;
        existingKeys.add(f.key);
        addedCount++;
    }

    // 新建分组
    const allZones = buildZones(dockFields);
    for (const [gid, keys] of Object.entries(allZones)) {
        if (!existingGroupIds.has(gid)) {
            let label = FALLBACK_GROUP_LABEL;
            for (const [, rgid, rlabel] of GROUP_RULES) {
                if (rgid === gid) {
                    label = rlabel;
                    break;
                }
            }
            existingGroups.push({ id: gid, label, keys });
        }
    }

    // 确保 FALLBACK_GROUP 列出未匹配字段
    if (!existingGroupIds.has(FALLBACK_GROUP_ID) && allZones[FALLBACK_GROUP_ID]) {
        existingGroups.push({
            id: FALLBACK_GROUP_ID,
            label: FALLBACK_GROUP_LABEL,
            keys: allZones[FALLBACK_GROUP_ID]
        });
    }

    console.log(`  新增字段: ${addedCount}`);
    console.log(`  已存在(跳过): ${skippedCount}`);
    console.log(`  总字段数: ${Object.keys(existingFields).length}`);
    console.log(`  总分组数: ${existingGroups.length}`);

    // 构建输出（保持 sort_keys 等效：对 key 排序）
    const sortedFields = {};
    for (const key of Object.keys(existingFields).sort()) {
        sortedFields[key] = existingFields[key];
    }

    return {
        topics: {
            ...existingMappings.topics,
            [TOPIC_KEY]: {
                description: topic.description || "机场/无人机 OSD 遥测数据 (0.5Hz 定时上报)",
                fields: sortedFields,
                groups: existingGroups
            }
        }
    };
}

// ============================================================
// 主入口
// ============================================================

function main() {
    const dryRun = process.argv.includes('--dry-run');

    // 1. 解析 Markdown
    console.log(`📖 解析: ${MD_PATH}`);
    const dockFields = parseDockOsdMd(MD_PATH);
    console.log(`  解析到 ${dockFields.length} 个字段`);

    // 2. 加载现有 JSON
    console.log(`📂 加载: ${JSON_PATH}`);
    let existing = {};
    if (fs.existsSync(JSON_PATH)) {
        existing = JSON.parse(fs.readFileSync(JSON_PATH, 'utf-8'));
    } else {
        console.log('  ⚠ JSON 文件不存在，将从头构建');
    }

    // 3. 合并
    console.log('🔄 合并中...');
    const updated = mergeFields(dockFields, existing);

    // 4. 输出
    const outputJson = JSON.stringify(updated, null, 2) + '\n';

    if (dryRun) {
        console.log('\n📋 --dry-run 模式，不写入文件');
        console.log(`  输出 JSON 长度: ${outputJson.length} 字符`);
        return;
    }

    console.log(`💾 写入: ${JSON_PATH}`);
    // 安全写入：先写临时文件
    const tmpPath = JSON_PATH.replace(/\.json$/, '.tmp');
    try {
        fs.writeFileSync(tmpPath, outputJson, 'utf-8');
        fs.renameSync(tmpPath, JSON_PATH);
        console.log('  ✓ 写入成功');
    } catch (e) {
        console.error(`  ✗ 写入失败: ${e.message}`);
        if (fs.existsSync(tmpPath)) fs.unlinkSync(tmpPath);
        process.exit(1);
    }
}

main();
