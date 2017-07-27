#!/bin/bash

echo -n "Creating animation"
NAME=$(basename $0 .sh)
TMPDIR=$(mktemp -d)
cd $(dirname $0)

INFILES=()
for (( w = 0; w <= 150; w += 10 )); do
	if [[ w -gt 148 ]]; then w=148; fi
	echo -n "."
	if [[ w -eq 0 ]]; then
		:
	elif [[ w -eq 1 ]]; then
		convert -geometry "1x18!" "$NAME.bar-left.png" \
		-mosaic "$TMPDIR/$w.png"
	elif [[ w -eq 2 ]]; then
		convert -geometry "1x18!" "$NAME.bar-left.png" \
		-geometry "1x18!" -page "+1+0" "$NAME.bar-right.png" \
		-mosaic "$TMPDIR/$w.png"
	elif [[ w -eq 3 ]]; then
		convert "$NAME.bar-left.png" \
		-geometry "1x18!" -page "+2+0" "$NAME.bar-right.png" \
		-mosaic "$TMPDIR/$w.png"
	elif [[ w -eq 4 ]]; then
		convert "$NAME.bar-left.png" \
		-page "+2+0" "$NAME.bar-right.png" \
		-mosaic "$TMPDIR/$w.png"
	else
		convert "$NAME.bar-left.png" \
		-geometry "$((w - 4))x18!" -page "+2+0" "$NAME.bar.png" \
		-page "+$((w - 2))+0" "$NAME.bar-right.png" \
		-mosaic "$TMPDIR/$w.png"
	fi
	if [[ w -eq 0 ]]; then
		cp "$NAME.base.png" "$TMPDIR/$w.png"
		INFILES=(-delay 20 "$TMPDIR/$w.png")
	else
		INFILES=("${INFILES[@]}" -page "+266+454" -dispose None -delay 10 "$TMPDIR/$w.png")
	fi
done

convert "${INFILES[@]}" -loop 1 "$NAME.gif"
rm $TMPDIR/*.png
rmdir $TMPDIR
echo "done!"
