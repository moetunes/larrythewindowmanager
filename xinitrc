starter="exec dbus-launch --sh-syntax --exit-with-session "

numlockx &
xsetroot -cursor_name left_ptr &
fbsetbg -l &
#turn of the caps lock key !!
xmodmap -e 'keycode 66 ='
#xterm &
urxvtd -q -f -o

# conky -c ~/build/status_conky.ssb | while read -r; do xsetroot -name "$REPLY"; done &
~/progs/c.files/status_output/dmini_stat/dministat_output &

ff="/tmp/larry.fifo.$DISPLAY"
[[ -p $ff ]] || mkfifo -m 600 "$ff"

ds=(" Ë" " ¾" " ´" " ©" " Ê" " à" " ¸" " ¹" " Æ" " Ç")

ms=("" "" "" "" "")

while read -r wmout || true; do
    if [[ $wmout == [[:digit:]]:[[:digit:]]:[[:digit:]]* ]]; then
        unset r
        read -ra desktops <<< "$wmout"
        for desktop in "${desktops[@]}"; do
            IFS=':' read -r d n c <<< "$desktop"
            fg="&B0&5"
            [[ $n -gt 0 ]] && fg="&B2&6"
            [[ $c -gt 0 ]] && fg="&B3&9" && num=" &1:&5$n"
            [[ "$c" != "" ]] && r+="$fg${ds[$d]} &5&B0\u19"
        done
        [[ "$desktop" = "1" ]] || num=""
        mode="${ms[$desktop]}"
        printf "$r&B0$num &1$mode \n"
    fi

done < $ff | ~/progs/c.files/bipolarbar/bipolarbar &

$starter ~/build/larry/larrythewindowmanager > $ff
# $starter ~/build/PKGs/dminiwm/dminiwm > $ff
