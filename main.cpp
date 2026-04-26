/*
 * ============================================================
 *  Digital Complaint Prioritization System for Municipal Services
 *  Course: Project Based Learning - II (CSE20090)
 *  by Kanishka Malpani
 *  Panel: F | SY.CSE | MIT World Peace University, Pune
 * ============================================================
 *
 * DATA STRUCTURES & ALGORITHMS USED:
 *   1. Max-Heap (Priority Queue) - O(log n) insert/extract
 *   2. Graph (Adjacency List) - location proximity
 *   3. Dijkstra's Algorithm - shortest path between zones
 *   4. Hash Map (unordered_map) - O(1) lookup for zones
 *   5. Greedy Algorithm - composite score computation
 *   6. Merge Sort - batch sorting complaints by score
 */

#include <iostream>
#include <queue>
#include <vector>
#include <unordered_map>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <climits>
#include <fstream>

using namespace std;

// ─────────────────────────────────────────────
//  CONSTANTS
// ─────────────────────────────────────────────
const int INF = INT_MAX;

// Urgency weights (tunable)
const int BASE_WATER       = 10;
const int BASE_ELECTRICITY = 8;
const int BASE_ROAD        = 6;
const int BASE_SANITATION  = 7;
const int BASE_OTHER       = 4;

// Penalty per hour the complaint is unresolved
const float TIME_DECAY     = 0.5f;

// Bonus if complaint is in a high-density zone
const int DENSITY_BONUS    = 3;

// ─────────────────────────────────────────────
//  ENUMS
// ─────────────────────────────────────────────
enum ComplaintType { WATER, ELECTRICITY, ROAD, SANITATION, OTHER };
enum Severity      { LOW = 1, MEDIUM = 2, HIGH = 3, CRITICAL = 4 };

// ─────────────────────────────────────────────
//  COMPLAINT STRUCTURE
// ─────────────────────────────────────────────
struct Complaint {
    int         id;
    string      citizenName;
    ComplaintType type;
    Severity    severity;
    string      zone;          // e.g., "Zone_A"
    time_t      timestamp;     // when it was filed
    float       priorityScore; // computed composite score
    bool        resolved;

    // Constructor
    Complaint(int id, string name, ComplaintType t, Severity s, string z)
        : id(id), citizenName(name), type(t), severity(s),
          zone(z), resolved(false) {
        timestamp = time(nullptr);
        priorityScore = 0;
    }

    // Helper: type as string
    string typeStr() const {
        switch(type){
            case WATER:       return "Water Supply";
            case ELECTRICITY: return "Electricity";
            case ROAD:        return "Road/Pothole";
            case SANITATION:  return "Sanitation";
            default:          return "Other";
        }
    }

    // Helper: severity as string
    string severityStr() const {
        switch(severity){
            case LOW:      return "Low";
            case MEDIUM:   return "Medium";
            case HIGH:     return "High";
            case CRITICAL: return "Critical";
            default:       return "Unknown";
        }
    }
};

// ─────────────────────────────────────────────
//  COMPARATOR for Max-Heap
//  Higher priorityScore comes out first
// ─────────────────────────────────────────────
struct CompareComplaint {
    bool operator()(const Complaint* a, const Complaint* b) {
        return a->priorityScore < b->priorityScore; // max-heap
    }
};

// ─────────────────────────────────────────────
//  GRAPH — Zones as nodes, roads as edges
//  Used by Dijkstra to find closest maintenance team
// ─────────────────────────────────────────────
class CityGraph {
public:
    unordered_map<string, vector<pair<string,int>>> adj;

    void addEdge(const string& u, const string& v, int dist) {
        adj[u].push_back({v, dist});
        adj[v].push_back({u, dist});
    }

    // Dijkstra from a source zone
    unordered_map<string, int> dijkstra(const string& src) {
        unordered_map<string, int> dist;
        for (auto& [zone, _] : adj) dist[zone] = INF;
        dist[src] = 0;

        // min-heap: {distance, zone}
        priority_queue<pair<int,string>,
                       vector<pair<int,string>>,
                       greater<>> pq;
        pq.push({0, src});

        while (!pq.empty()) {
            auto [d, u] = pq.top(); pq.pop();
            if (d > dist[u]) continue;
            for (auto& [v, w] : adj[u]) {
                if (dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    pq.push({dist[v], v});
                }
            }
        }
        return dist;
    }
};

// ─────────────────────────────────────────────
//  PRIORITY SCORE CALCULATOR (Greedy Approach)
//  Score = base_type_weight
//        + severity * 5
//        + time_elapsed_hours * TIME_DECAY
//        + density_bonus (if high-density zone)
// ─────────────────────────────────────────────
float calcScore(const Complaint& c,
                const unordered_map<string,bool>& highDensityZones)
{
    int base = 0;
    switch (c.type) {
        case WATER:       base = BASE_WATER;       break;
        case ELECTRICITY: base = BASE_ELECTRICITY; break;
        case ROAD:        base = BASE_ROAD;        break;
        case SANITATION:  base = BASE_SANITATION;  break;
        default:          base = BASE_OTHER;       break;
    }

    float severityWeight = c.severity * 5.0f;

    double elapsed = difftime(time(nullptr), c.timestamp) / 3600.0; // hours
    float timeBonus = (float)elapsed * TIME_DECAY;

    float densityBonus = 0;
    auto it = highDensityZones.find(c.zone);
    if (it != highDensityZones.end() && it->second)
        densityBonus = DENSITY_BONUS;

    return base + severityWeight + timeBonus + densityBonus;
}

// ─────────────────────────────────────────────
//  MERGE SORT — sort vector of complaints by score
// ─────────────────────────────────────────────
void merge(vector<Complaint*>& arr, int l, int m, int r) {
    vector<Complaint*> left(arr.begin()+l, arr.begin()+m+1);
    vector<Complaint*> right(arr.begin()+m+1, arr.begin()+r+1);
    int i=0, j=0, k=l;
    while (i < (int)left.size() && j < (int)right.size()) {
        if (left[i]->priorityScore >= right[j]->priorityScore)
            arr[k++] = left[i++];
        else
            arr[k++] = right[j++];
    }
    while (i < (int)left.size())  arr[k++] = left[i++];
    while (j < (int)right.size()) arr[k++] = right[j++];
}

void mergeSort(vector<Complaint*>& arr, int l, int r) {
    if (l < r) {
        int m = (l+r)/2;
        mergeSort(arr, l, m);
        mergeSort(arr, m+1, r);
        merge(arr, l, m, r);
    }
}

// ─────────────────────────────────────────────
//  COMPLAINT MANAGEMENT SYSTEM
// ─────────────────────────────────────────────
class ComplaintSystem {
    priority_queue<Complaint*,
                   vector<Complaint*>,
                   CompareComplaint>   heap;     // for live prioritization
    vector<Complaint*>                 allComplaints;
    unordered_map<string, bool>        highDensityZones;
    CityGraph                          cityGraph;
    int                                nextId = 1;

public:
    ComplaintSystem() {
        // Mark high-density zones
        highDensityZones["Zone_A"] = true;
        highDensityZones["Zone_B"] = true;
        highDensityZones["Zone_C"] = false;
        highDensityZones["Zone_D"] = false;
        highDensityZones["Zone_E"] = true;

        // Build city graph (zones and inter-zone distances in km)
        cityGraph.addEdge("Zone_A", "Zone_B", 2);
        cityGraph.addEdge("Zone_B", "Zone_C", 3);
        cityGraph.addEdge("Zone_C", "Zone_D", 1);
        cityGraph.addEdge("Zone_D", "Zone_E", 4);
        cityGraph.addEdge("Zone_A", "Zone_E", 6);
        cityGraph.addEdge("Zone_B", "Zone_D", 5);
    }

    ~ComplaintSystem() {
        for (auto c : allComplaints) delete c;
    }


    // ── Add new complaint ──
    int addComplaint(string name, ComplaintType type,
                     Severity sev, string zone,time_t customTime=0)
    {
        Complaint* c = new Complaint(nextId++, name, type, sev, zone);
            if(customTime != 0) c->timestamp = customTime; // For demo purposes
            else c->timestamp = customTime;
        c->priorityScore = calcScore(*c, highDensityZones);
        allComplaints.push_back(c);
        heap.push(c);

        cout << "\n[+] Complaint #" << c->id
             << " (" << c->typeStr() << ") filed by " << name
             << " | Zone: " << zone
             << " | Severity: " << c->severityStr()
             << " | Score: " << fixed << setprecision(2) << c->priorityScore
             << "\n";
        return c->id;
    }

    // ── Process top-priority complaint ──
    void processNext() {
        // Rebuild heap with updated scores (time-based)
        while (!heap.empty()) heap.pop();
        for (auto c : allComplaints) {
            if (!c->resolved) {
                c->priorityScore = calcScore(*c, highDensityZones);
                heap.push(c);
            }
        }

        if (heap.empty()) {
            cout << "\n[!] No pending complaints.\n";
            return;
        }

        Complaint* top = heap.top(); heap.pop();
        top->resolved = true;

        cout << "\n[✓] PROCESSING: Complaint #" << top->id
             << " | " << top->typeStr()
             << " | Zone: " << top->zone
             << " | Score: " << fixed << setprecision(2) << top->priorityScore
             << " | Citizen: " << top->citizenName << "\n";
    }

    // ── Show all pending complaints sorted by priority ──
    void showPriorityQueue() {
        vector<Complaint*> pending;
        for (auto c : allComplaints)
            if (!c->resolved) {
                c->priorityScore = calcScore(*c, highDensityZones);
                pending.push_back(c);
            }

        if (pending.empty()) {
            cout << "\n[!] No pending complaints.\n";
            return;
        }

        mergeSort(pending, 0, (int)pending.size()-1);

        cout << "\n" << string(70, '=') << "\n";
        cout << "  COMPLAINT PRIORITY QUEUE (Highest → Lowest)\n";
        cout << string(70, '=') << "\n";
        cout << left
             << setw(5)  << "Rank"
             << setw(6)  << "ID"
             << setw(20) << "Type"
             << setw(10) << "Severity"
             << setw(10) << "Zone"
             << setw(8)  << "Score"
             << "Citizen\n";
        cout << string(70, '-') << "\n";

        int rank = 1;
        for (auto c : pending) {
            cout << left
                 << setw(5)  << rank++
                 << setw(6)  << c->id
                 << setw(20) << c->typeStr()
                 << setw(10) << c->severityStr()
                 << setw(10) << c->zone
                 << setw(8)  << fixed << setprecision(1) << c->priorityScore
                 << c->citizenName << "\n";
        }
        cout << string(70, '=') << "\n";
    }

    // ── Dijkstra: find closest maintenance zone ──
    void findClosestTeam(const string& fromZone) {
        cout << "\n[MAP] Shortest distances from " << fromZone << ":\n";
        auto dists = cityGraph.dijkstra(fromZone);
        // Sort by distance
        vector<pair<int,string>> sorted;
        for (auto& [z, d] : dists)
            sorted.push_back({d, z});
        sort(sorted.begin(), sorted.end());

        for (auto& [d, z] : sorted) {
            cout << "  " << fromZone << " → " << z
                 << " : " << (d == INF ? "∞" : to_string(d)) << " km\n";
        }
    }

    // ── Statistics ──
void showStats() {
    int total=0, resolved=0, water=0, elec=0, road=0, san=0, oth=0;

    for(auto c : allComplaints) {
        total++;
        if(c->resolved) resolved++;

        switch(c->type) {
            case WATER: water++; break;
            case ELECTRICITY: elec++; break;
            case ROAD: road++; break;
            case SANITATION: san++; break;
            default: oth++;
        }
    }

    ofstream file("data.json");

    file << "{\n";
    file << "  \"water\": " << water << ",\n";
    file << "  \"electricity\": " << elec << ",\n";
    file << "  \"road\": " << road << ",\n";
    file << "  \"sanitation\": " << san << ",\n";
    file << "  \"other\": " << oth << ",\n";
    file << "  \"resolved\": " << resolved << ",\n";

    // 🔥 ADD THIS PART
    file << "  \"complaints\": [\n";

    bool first = true;
    for(auto c : allComplaints) {
        if(!c->resolved) {
            if(!first) file << ",\n";
            first = false;

            int severity;

if(c->priorityScore < 18)
    severity = 0;        // LOW
else if(c->priorityScore < 23)
    severity = 1;        // MEDIUM
else if(c->priorityScore < 28)
    severity = 2;        // HIGH
else
    severity = 3;        // CRITICAL

            file << "    {";
            file << "\"id\": " << c->id << ",";
            file << "\"name\": \"" << c->citizenName << "\",";
            file << "\"zone\": \"" << c->zone << "\",";
            file << "\"type\": " << c->type << ",";
                        file << "\"severity\": " << severity << ",";

            file << "\"score\": " << c->priorityScore;
            file << "}";
        }
    }

    file << "\n  ]\n";
    file << "}";

    file.close();

    cout << "✅ data.json updated\n";
}
};

// ─────────────────────────────────────────────
//  INTERACTIVE MENU
// ─────────────────────────────────────────────
void showMenu() {
  cout << "\n╔══════════════════════════════════════════╗\n";
    cout << "║  DIGITAL COMPLAINT PRIORITIZATION SYSTEM ║\n";
    cout << "║  Municipal Services — MIT WPU, Pune      ║\n";
    cout << "╚══════════════════════════════════════════╝\n";
    cout << "  1. File New Complaint\n";
    cout << "  2. View Priority Queue\n";
    cout << "  3. Process Highest Priority Complaint\n";
    cout << "  4. Find Closest Maintenance Team (Dijkstra)\n";
    cout << "  5. Show Statistics\n";
    cout << "  6. Run Demo (auto-load sample complaints)\n";
    cout << "  0. Exit\n";
    cout << "  Choice: ";
}

ComplaintType getType() {
    cout << "  Types: 1=Water  2=Electricity  3=Road  4=Sanitation  5=Other\n";
    cout << "  Enter type: ";
    int t; cin >> t;
    return (ComplaintType)(t-1);
}

Severity getSeverity() {
    cout << "  Severities: 1=Low  2=Medium  3=High  4=Critical\n";
    cout << "  Enter severity: ";
    int s; cin >> s;
    return (Severity)s;
}

// ─────────────────────────────────────────────
//  DEMO — pre-loads realistic complaints
// ─────────────────────────────────────────────
void runDemo(ComplaintSystem& sys) {
    cout << "\n[DEMO] Loading sample complaints...\n";

    time_t now = time(0);

    sys.addComplaint("Rahul Sharma", ROAD, CRITICAL, "Zone_A", now - 300);
    sys.addComplaint("Priya Iyer", ELECTRICITY, HIGH, "Zone_B", now - 7200);
    sys.addComplaint("Amit Desai", ROAD, MEDIUM, "Zone_C", now - 15000);
    sys.addComplaint("Sunita Patil", SANITATION, HIGH, "Zone_A", now - 30000);
    sys.addComplaint("Deepak Kulkarni", ROAD, HIGH, "Zone_D", now - 1800);
    sys.addComplaint("Meena Joshi", ELECTRICITY, LOW, "Zone_E", now - 9000);
    sys.addComplaint("Vinod Nair", ROAD, CRITICAL, "Zone_B", now - 20000);
    sys.addComplaint("Kavya Reddy", WATER, MEDIUM, "Zone_C", now - 40000);
}
// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
int main() {
    ComplaintSystem system;

    //  AUTO LOAD DATA AT START
    runDemo(system);

    //  CREATE JSON IMMEDIATELY
    system.showStats();

    int choice;

    do {
        showMenu();
        cin >> choice;

        switch(choice) {
            case 1: {
                cin.ignore();
                string name, zone;
                cout << "  Citizen name: "; getline(cin, name);
                ComplaintType t = getType();
                Severity s = getSeverity();
                cout << "  Zones available: Zone_A  Zone_B  Zone_C  Zone_D  Zone_E\n";
                cout << "  Zone: "; cin >> zone;

                system.addComplaint(name, t, s, zone);

                //  AUTO UPDATE DASHBOARD
                system.showStats();
                break;
            }

            case 2:{
                system.showPriorityQueue();
                break;
            }
            case 3:
                {system.processNext();

                //  AUTO UPDATE DASHBOARD
                system.showStats();
                break;
                }
            case 4: {
                string z;
                cout << "  Enter zone (Zone_A to Zone_E): "; cin >> z;
                system.findClosestTeam(z);
                break;
            }

            case 5:
                {system.showStats();
                break;
                }
            case 6:
                {   runDemo(system);

                // ALSO UPDATE JSON
                system.showStats();

                system.showPriorityQueue();
                break;
                }
            case 0:
                {cout << "\n  Goodbye! Exiting system.\n\n";
                break;
                }
            default:
                cout << "\n  Invalid choice.\n";
            
            }
    } while(choice != 0);

    return 0;
}