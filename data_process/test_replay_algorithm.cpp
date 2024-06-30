#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int main() {
    vector<int> timestamp = {1, 3, 25, 28, 50, 100, 110, 111, 115, 130};
    const int interval = 4;
    const int n = timestamp.size();
    int ** nxt_range = new int*[n];
    int ** nxt_value = new int*[n];
    int ** nxt_pkt = new int*[n];
    int ** nxt_flow = new int*[n];
    vector<int>* nxt_sametime_pkt = new vector<int>[n];
    vector<int>* nxt_sametime_flow = new vector<int>[n];
    for (int i = 0; i < n; i++) {
        nxt_range[i] = new int[n];
        nxt_value[i] = new int[n];
        nxt_pkt[i] = new int[n];
        nxt_flow[i] = new int[n];
        for (int j = 0; j < n; j++) {
            nxt_range[i][j] = 0;
            nxt_value[i][j] = 0;
            nxt_pkt[i][j] = 0;
            nxt_flow[i][j] = 0;
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            nxt_range[i][j] = -((timestamp[j] - timestamp[i])/interval);
            nxt_value[i][j] = (timestamp[j] - timestamp[i]) % interval;
            if(nxt_value[i][j] < 0)
            {
                nxt_value[i][j] += interval;
                nxt_range[i][j]++;
            }
        }
    }
    for(int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if(nxt_value[i][j] == 0)
            {
                nxt_sametime_pkt[i].push_back(j);
                nxt_sametime_flow[i].push_back(nxt_range[i][j]);
            }
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            cout << "(" << nxt_range[i][j] << " " << nxt_value[i][j] << ")" << "\t";
        }
        cout << endl;
    }
    for(int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if(j > i)
            {
                int min_interval = interval;
                int min_pktidx = n;
                nxt_pkt[i][j] = -1;
                nxt_flow[i][j] = 0;
                for (int k = i+1; k <= j; k++)
                {
                    if(nxt_value[i][k] == 0)
                    {
                        min_interval = 0;
                        nxt_pkt[i][j] = k;
                        nxt_flow[i][j] = nxt_range[i][k];
                        break;
                    }
                    else if(nxt_value[i][k] < min_interval)
                    {
                        min_interval = nxt_value[i][k];
                        nxt_pkt[i][j] = k;
                        nxt_flow[i][j] = nxt_range[i][k];
                    }
                }
                nxt_value[i][j] = min_interval;
            }
            else
            {
                int min_interval = interval;
                nxt_pkt[i][j] = -1;
                nxt_flow[i][j] = 0;
                for(int k = j; k < i; k++){
                    if(nxt_value[i][k] == 0)
                    {
                        continue;
                    }
                    else if(nxt_value[i][k] < min_interval)
                    {
                        min_interval = nxt_value[i][k];
                        nxt_pkt[i][j] = k;
                        nxt_flow[i][j] = nxt_range[i][k];
                    }
                }
                nxt_value[i][j] = min_interval;
            }
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            cout << "(" << nxt_range[i][j] << " " << nxt_value[i][j] << " " << nxt_pkt[i][j] << " " << nxt_flow[i][j] << ")" << "\t";
        }
        cout << endl;
    }
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < nxt_sametime_flow[i].size(); j++) {
            cout << "(" << nxt_sametime_pkt[i][j] << " " << nxt_sametime_flow[i][j] << ")" << "\t";
        }
        cout << endl;
    }

    int total_pkt = 500 * timestamp.size();

    int up_bound = 0;
    int down_bound = 499;
    int flow_idx = 0;
    int pkt_idx = 0;
    int time = 1;

    vector<int> time_send;
    time_send.push_back(1);
    for (int _ = 0; _ < total_pkt; _++) {
        int up_range = flow_idx - up_bound;
        int down_range = down_bound - flow_idx;
        int tmp_pkt_idx = n;
        int tmp_flow_idx = 0;
        int tmp_value = interval;
        for(int i = n-1; i > pkt_idx; i--) {
            if (-up_range <= nxt_range[pkt_idx][i] && nxt_pkt[pkt_idx][i] != -1 && (nxt_value[pkt_idx][i] < tmp_value || (nxt_value[pkt_idx][i] == tmp_value && nxt_pkt[pkt_idx][i] < tmp_pkt_idx))) {
                tmp_pkt_idx = nxt_pkt[pkt_idx][i];
                tmp_flow_idx = flow_idx + nxt_flow[pkt_idx][i];
                tmp_value = nxt_value[pkt_idx][i];
                break;
            }
        }
        for(int i = 0; i < pkt_idx; i++) {
            if (down_range >= nxt_range[pkt_idx][i] && nxt_pkt[pkt_idx][i] != -1 && (nxt_value[pkt_idx][i] < tmp_value || (nxt_value[pkt_idx][i] == tmp_value && nxt_pkt[pkt_idx][i] < tmp_pkt_idx))) {
                tmp_pkt_idx = nxt_pkt[pkt_idx][i];
                tmp_flow_idx = flow_idx + nxt_flow[pkt_idx][i];
                tmp_value = nxt_value[pkt_idx][i];
                break;
            }
        }
        if (tmp_value >= interval)
        {
            tmp_pkt_idx = pkt_idx;
            tmp_flow_idx = flow_idx + 1;
            down_range = down_bound - tmp_flow_idx;
            for(int i = 0; nxt_sametime_flow[tmp_pkt_idx][i] >= 0; i++) {
                if(down_range >= nxt_sametime_flow[tmp_pkt_idx][i]) {
                    flow_idx = tmp_flow_idx + nxt_sametime_flow[tmp_pkt_idx][i];
                    time += interval;
                    pkt_idx = nxt_sametime_pkt[tmp_pkt_idx][i];
                    time_send.push_back(time);
                    break;
                }
            }
        }
        else if (tmp_value != 0){
            down_range = down_bound - tmp_flow_idx;
            for(int i = 0; nxt_sametime_flow[tmp_pkt_idx][i] >= 0; i++) {
                if(down_range >= nxt_sametime_flow[tmp_pkt_idx][i]) {
                    flow_idx = tmp_flow_idx + nxt_sametime_flow[tmp_pkt_idx][i];
                    time += tmp_value;
                    pkt_idx = nxt_sametime_pkt[tmp_pkt_idx][i];
                    time_send.push_back(time);
                    break;
                }
            }
        }
        else {
            pkt_idx = tmp_pkt_idx;
            flow_idx = tmp_flow_idx;
            time += tmp_value;
            time_send.push_back(time);
        }
    }
    // unique(time_send.begin(), time_send.end());
    printf("time_send: ");
    for (int i = 0; i < time_send.size(); i++) {
        printf("%d ", time_send[i]);
    }
    vector<int> ans;
    for (int i = 0; i < 500; i++) {
        for (int j = 0; j < timestamp.size(); j++) {
            ans.push_back(timestamp[j]+(i)*interval);
        }
    }
    sort(ans.begin(), ans.end());
    // unique(ans.begin(), ans.end());
    printf("ans: ");
    for (int i = 0; i < ans.size(); i++) {
        printf("%d ", ans[i]);
    }
    for(int i = 0; i < ans.size(); i++) {
        if(ans[i] != time_send[i])
        {
            printf("error : %d %d\n", ans[i], time_send[i]);
            break;
        }
    }
    return 0;
}