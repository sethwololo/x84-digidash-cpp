// Converts the SVGs in assets/images into LVGL images (components/ui/images/<name>.c), at the size each SVG
// declares. They are A8: only how opaque each pixel is. The color comes from the widget's style (image_recolor),
// so one image serves every color it is shown in.
//
//   cd tools && npm install && node images.mjs
import { readdirSync, readFileSync, writeFileSync } from 'node:fs'
import { basename } from 'node:path'
import { Resvg } from '@resvg/resvg-js'

const source = new URL('../assets/images/', import.meta.url)
const target = new URL('../components/ui/images/', import.meta.url)

for (const file of readdirSync(source).filter((name) => name.endsWith('.svg'))) {
  const name = `img_${basename(file, '.svg').replaceAll('-', '_')}`
  const rendered = new Resvg(readFileSync(new URL(file, source)), { fitTo: { mode: 'original' } }).render()
  const { width, height, pixels } = rendered
  const alpha = []
  for (let i = 3; i < pixels.length; i += 4) alpha.push(pixels[i])
  const rows = []
  for (let y = 0; y < height; y++) rows.push('    ' + alpha.slice(y * width, (y + 1) * width).join(',') + ',')
  writeFileSync(
    new URL(`${name}.c`, target),
    `// ${file}, converted by tools/images.mjs: ${width}x${height}, A8.
#include "lvgl.h"

static const uint8_t ${name}_map[] = {
${rows.join('\n')}
};

const lv_image_dsc_t ${name} = {
    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_A8, .w = ${width}, .h = ${height}, .stride = ${width}},
    .data_size = sizeof(${name}_map),
    .data = ${name}_map,
};
`,
  )
  console.log(`${name}: ${width}x${height}`)
}
