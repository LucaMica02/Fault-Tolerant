import csv
import matplotlib.pyplot as plt
import numpy as np

# TODO: Gestire segfault

'''
['N', 'DELAY', 'BUF SIZE', 'KILLED', 'TIME', 'DEADLOCK', 'SEGFAULT', 'ABORT', 'RIGHT RESULT']
  0      1          2         3        4         5           6          7            8     
'''

def plotLog(filename, title):
    N = []
    KILLED = []
    total_rows = 0
    useful_rows = 0
    deadlock = 0
    abort = 0
    segfault = 0
    wrongResult = 0
    ok_abort = 0
    ok = 0

    with open(filename, mode='r') as file:
        reader = csv.reader(file)
        for i, row in enumerate(reader):
            if i == 0:
                continue
            row = row[0].split(';')
            survived = int(row[0]) - int(row[3])
            # if don't happen a kill skip the row
            if row[3] != '0': 
                # deadlock
                if row[5] == 'True':
                    deadlock += 1
                # wrong result
                elif row[8] == 'False':
                    wrongResult += 1
                # abort
                elif row[7] == 'True':
                    if survived > 0:
                        ok_abort += 1
                        N.append(int(row[0]))
                        KILLED.append(int(row[3]))
                    else:
                        abort += 1
                # kill but ok
                else:
                    ok += 1
                    N.append(int(row[0]))
                    KILLED.append(int(row[3]))
                useful_rows += 1
            total_rows += 1

    deadlock_perc = (deadlock / useful_rows) * 100
    abort_perc = (abort / useful_rows) * 100
    wrongResult_perc = (wrongResult / useful_rows) * 100
    ok_perc = (ok / useful_rows) * 100
    ok_abort_perc = (ok_abort / useful_rows) * 100

    N_avg = round((sum(N) / len(N)), 2)
    N_stdd = round(np.std(N, ddof=1), 2)
    KILLED_avg = round((sum(KILLED) / len(KILLED)), 2)
    KILLED_stdd = round(np.std(KILLED, ddof=1), 2)

    #"""
    print("DEADLOCK: %", deadlock_perc)
    print("WRONG RESULT: %", wrongResult_perc)
    print("ABORT: %", abort_perc)
    print("OK: %", ok_perc)
    print("OK ABORT: %", ok_abort_perc)
    print("Total rows:", total_rows)
    print("Useful Rows:", useful_rows)
    print("N AVG", N_avg)
    print("KILLED AVG", KILLED_avg)
    print("N STDD", N_stdd)
    print("KILLED STDD", KILLED_stdd)
   #"""

    # Data to plot
    labels = ["Deadlock", "Wrong Result", "Abort", "OK", "OK Abort"]
    sizes = [deadlock_perc, wrongResult_perc, abort_perc, ok_perc, ok_abort_perc]

    # Create a simple pie chart
    wedges, texts, autotexts = plt.pie(sizes, labels=labels, autopct='%1.1f%%', center=(-1, 0))

    # Create custom labels for the legend
    custom_labels = [f'{label} - {round(size, 2)}%' for label, size in zip(labels, sizes)]
    plt.legend(wedges, custom_labels, loc="right", bbox_to_anchor=(1.5, 0.8))

    # Plot the text info about the rows
    plt.text(
        0.6, -1, 
        f"Total Rows: {total_rows}\nUseful Rows: {useful_rows}", 
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
    plt.title(title)
    plt.show()

plotLog('../log/log_single_RD.csv', "RD single kill")
print("################################################")
plotLog('../log/log_multiple_RD.csv', "RD multiple kill")
print("################################################")
plotLog('../log/log_single_Raben.csv', "Raben single kill")