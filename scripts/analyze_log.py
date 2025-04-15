import csv
import matplotlib.pyplot as plt
import numpy as np

'''
['N', 'DELAY', 'THRESHOLD', 'BUF SIZE', 'KILLED DOCKER', 'REAL MPI KILLED', 'TIME', 'DEADLOCK', 'SEGFAULT', 'ABORT', 'RIGHT RESULT']
  0      1          2            3             4                 5             6         7           8          9           10         
'''
N = []
KILLED = []
total_rows = 0
rows = 0
deadlock = 0
#segfault = 0
abort = 0
noKill = 0
wrongResult = 0
ok = 0

with open('../log2.csv', mode='r') as file:
    reader = csv.reader(file)
    for i, row in enumerate(reader):
        if rows != 0:
            row = row[0].split(';')
            survived = int(row[0]) - int(row[5])
            # if don't happen a kill skip the row
            if row[5] != '0': 
                # deadlock
                if row[7] == 'True':
                    deadlock += 1
                # abort
                elif row[9] == 'True':
                    if row[10] == 'True' and survived > 0:
                        ok += 1
                        N.append(int(row[0]))
                        KILLED.append(int(row[5]))
                    else:
                        abort += 1
                # wrong result
                elif row[10] == 'False':
                    wrongResult += 1
                else:
                    ok += 1
                    N.append(int(row[0]))
                    KILLED.append(int(row[5]))
                rows += 1
        else:
            rows += 1
        total_rows += 1

deadlock_perc = (deadlock / rows) * 100
#segfault_perc = (segfault / rows) * 100
abort_perc = (abort / rows) * 100
noKill_perc = (noKill / rows) * 100
wrongResult_perc = (wrongResult / rows) * 100
ok_perc = (ok / rows) * 100
N_avg = round((sum(N) / len(N)), 2)
N_stdd = round(np.std(N, ddof=1), 2)
KILLED_avg = round((sum(KILLED) / len(KILLED)), 2)
KILLED_stdd = round(np.std(KILLED, ddof=1), 2)

print("DEADLOCK: %", deadlock_perc)
#print("SEGFAULT: %", segfault_perc)
print("ABORT: %", abort_perc)
print("NO KILL: %", noKill_perc)
print("WRONG RESULT: %", wrongResult_perc)
print("OK: %", ok_perc)
print("Total rows:", total_rows)
print("Useful Rows:", rows)
print("N AVG", N_avg)
print("KILLED AVG", KILLED_avg)
print("N STDD", N_stdd)
print("KILLED STDD", KILLED_stdd)

# Data to plot
labels = ["Deadlock", "Abort", "Wrong Result", "OK"]
sizes = [deadlock_perc, abort_perc, wrongResult_perc, ok_perc]
#colors = ["blue", "black", "red", "white", "green"]

# Create a simple pie chart
wedges, texts, autotexts = plt.pie(sizes, labels=labels, autopct='%1.1f%%', center=(-1, 0))

# Create custom labels for the legend
custom_labels = [f'{label} - {round(size, 2)}%' for label, size in zip(labels, sizes)]
plt.legend(wedges, custom_labels, loc="right", bbox_to_anchor=(1.5, 0.8))

# Plot the text info about the rows
plt.text(
    0.6, -1, 
    f"Total Rows: {total_rows}\nUseful Rows: {rows}", 
    fontsize=14, 
    ha='left', va='center',
    bbox=dict(facecolor='lightgrey', alpha=0.5, boxstyle='round,pad=0.5')
)

# Plot the text info about the avg values
plt.text(
    -3.8, 1, 
    f"N Average: {N_avg}\nN Std Dev: {N_stdd}\nKilled Average: {KILLED_avg}\nKilled Std Dev: {KILLED_stdd}", 
    fontsize=14, 
    ha='left', va='center',
    bbox=dict(facecolor='lightblue', alpha=0.5, boxstyle='round,pad=0.5')
)

# Draw a circle in the middle
centre_circle = plt.Circle((-1,0),0.5,fc='white')
fig = plt.gcf()
fig.gca().add_artist(centre_circle)

# Show it
plt.title('Log data distribution')
plt.show()

# CALCULATE STDEVs