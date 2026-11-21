# Example toolbar icons

Shared flat SVG icons for the toolbars of the Qwt example applications.
The resources are compiled into an example by adding this directory's
`icons.qrc` to the example target ( `AUTORCC` picks it up ) and are then
addressed as `:/icons/<name>.svg`.

```cpp
#include <QIcon>

button->setIcon( QIcon( ":/icons/export.svg" ) );
```

## Style rules

Every icon in this directory follows the same rules, so that new icons can be
added without a visual mismatch:

- **Canvas**: `24 x 24` viewBox, content kept inside a 2 px margin.
- **Format**: plain flat SVG, no raster images, no filters, no gradients.
  The only transparency allowed is a flat `fill-opacity` tint on a
  highlighted face ( e.g. `golden.svg`, `coord-box.svg` ).
- **Geometry**: stroked line art with `stroke-linecap="round"` and
  `stroke-linejoin="round"`. Usual stroke widths are `1.2 - 1.8`; neutral
  outlines stay at `1.3 - 1.5`, emphasized strokes at `1.6 - 1.8`.
- **Palette**: two colors carry the semantics, matching the icons of the
  `parasitePlot` example:

  | Color | Role |
  |-------|------|
  | `#9aa0a6` neutral gray | frame, container, secondary/second-axis geometry |
  | `#1f6feb` accent blue | the primary object or action of the icon |
  | `#2b2b2b` dark ink | content lines inside a neutral container ( text/align icons ) |

- **No text**: icons never contain `<text>` elements, letters or numbers.
  A symbol such as the question mark of `help.svg` is drawn as a path.
- **Meaning**: one primary object per icon; a second, secondary object is
  drawn in gray. Prefer the conventional glyph ( printer, camera, floppy,
  magnifier, home, trash ) over an idiosyncratic one.

The icons render at the default toolbar icon size, so the shape has to stay
readable at `16 x 16`: keep the number of separate strokes low and do not
encode information below one pixel.

## File naming

Lower case, hyphen separated, named after what the button does:
`export.svg`, `save.svg`, `title-x-outside.svg`, `reset-view.svg`.
Icon families use a common prefix: `coord-box`, `coord-frame`,
`coord-none`, `coord-style`; `export.svg`, `export-png.svg`,
`export-svg.svg`.

## Adding a new icon

1. Check whether an existing icon already carries the meaning; prefer reuse
   over a near duplicate.
2. Draw it with the same viewBox, stroke widths and palette as above.
3. Add the file to `icons.qrc` ( keep the list alphabetical ).
4. Use it from the example as `QIcon( ":/icons/<name>.svg" )`.

## Usage notes

- SVG icons are loaded through Qt's SVG image format plugin, i.e. the
  `QtSvg` module has to be installed with the Qt runtime used to run the
  examples ( Qwt itself only needs it for its optional SVG export ).
- Toolbars with mixed icon/text buttons should set a tool button style
  ( `Qt::ToolButtonTextBesideIcon` / `Qt::ToolButtonTextUnderIcon` ) and
  optionally `setIconSize( QSize( 20, 20 ) )` so that the label and the
  icon are both visible.
