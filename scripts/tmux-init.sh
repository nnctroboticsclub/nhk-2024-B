H=$(tmux display -p '#{window_height}')



tmux set-option -g default-terminal screen-256color
tmux set -g terminal-overrides 'xterm:colors=256'

tmux set-option -g status-position top
tmux set-option -g status-left-length 90
tmux set-option -g status-right-length 90

tmux set-option -g status-left "#S/#I:#W.#P"
tmux set-option -g status-right ''
tmux set-option -g status-justify centre
tmux set-option -g status-bg "colour238"
tmux set-option -g status-fg "colour255"

tmux bind '|' split-window -h
tmux bind '-' split-window -v

tmux set-option -g mouse on
tmux bind -n WheelUpPane if-shell -F -t = "#{mouse_any_flag}" "send-keys -M" "if -Ft= '#{pane_in_mode}' 'send-keys -M' 'copy-mode -e'"



tmux set -g mouse on

tmux split-window -t robomon.0 -h
tmux split-window -t robomon.0 -v
tmux split-window -t robomon.0 -v
tmux split-window -t robomon.0 -h
tmux split-window -t robomon.2 -h
tmux split-window -t robomon.4 -h

tmux resize-pane -t robomon.6 -x 80
tmux resize-pane -t robomon.0 -y $(($H / 4))
tmux resize-pane -t robomon.2 -y $(($H / 2))
tmux resize-pane -t robomon.4 -y $(($H / 4))

tmux setw synchronize-panes
tmux send-keys -t robomon.0 'cd ~/ghq/github.com/nnctroboticsclub/nhk-2024-b' C-m
tmux setw synchronize-panes

tmux send-keys -t robomon.6 '. .venv/bin/activate; make f_s' C-m
tmux send-keys -t robomon.1 'for f in /dev/ttyACM*; do stty -F $f $(cat .fep/ms_params); done' C-m
tmux send-keys -t robomon.2 'socat stdio /dev/ttyACM0' C-m
tmux send-keys -t robomon.3 'socat stdio /dev/ttyACM1' C-m
tmux send-keys -t robomon.5 'cd syoch-robotics' C-m
tmux send-keys -t robomon.5 'python3 -m utils.test' C-m
