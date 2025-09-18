import csv
import matplotlib.pyplot as plt
import numpy as np

'''
[N, DELAY, BUF_SIZE, TIMEOUT, TIME, DEADLOCK, DEADLOCK DETECTION, SEGFAULT, ABORT, RIGHT RESULT, REAL MPI KILLED]
 0    1       2         3      4        5             6               7       8         9              10
'''

def plotLog(filename, title):
    total_rows = 0
    rows = 0
    deadlock = 0
    deadlock_det = 0
    segfault = 0
    abort = 0
    wrongResult = 0
    ok = 0
    ok_abort = 0

    with open(filename, mode='r') as file:
        reader = csv.reader(file)
        for i, row in enumerate(reader):
            if rows != 0:
                row = row[0].split(';')
                survived = int(row[0]) - int(row[10])
                # if don't happen a kill skip the row
                if row[10] != '0': 
                    # deadlock
                    if row[5] == 'True':
                        deadlock += 1
                    if row[6] == 'True':
                        deadlock_det += 1
                    # segfault
                    elif row[7] == 'True':
                        if row[9] == 'False':
                            wrongResult += 1
                        else:
                            segfault += 1
                    # abort
                    elif row[8] == 'True':
                        if row[9] == 'False':
                            wrongResult += 1
                        elif survived > 0:
                            ok_abort += 1
                        else:
                            abort += 1
                    # wrong result
                    elif row[9] == 'False':
                        wrongResult += 1
                    else:
                        ok += 1
                    rows += 1
            else:
                rows += 1
            total_rows += 1

    deadlock_perc = (deadlock / rows) * 100
    deadlock_det_perc = (deadlock_det / rows) * 100
    segfault_perc = (segfault / rows) * 100
    abort_perc = (abort / rows) * 100
    wrongResult_perc = (wrongResult / rows) * 100
    ok_perc = (ok / rows) * 100
    ok_abort_perc = (ok_abort / rows) * 100
    print(deadlock, wrongResult)
    print("deadlock", deadlock_perc)
    print("deadlock detection", deadlock_det_perc)
    print("segfault", segfault_perc)
    print("abort", abort_perc)
    print("wrong result", wrongResult_perc)
    print("ok", ok_perc)
    print("ok ma abort", ok_abort_perc)
    
    # Data to plot
    labels = ["Deadlock", "Deadlock Detection", "Wrong Result", "Segfault", "Abort", "OK", "OK Abort"]
    sizes = [deadlock_perc, deadlock_det_perc, wrongResult_perc, segfault_perc, abort_perc, ok_perc, ok_abort_perc]

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

    # Draw a circle in the middle
    centre_circle = plt.Circle((-1,0),0.5,fc='white')
    fig = plt.gcf()
    fig.gca().add_artist(centre_circle)

    # Show it
    plt.title(title)
    plt.show()

plotLog('../logs/log.csv', "Raben (rs + all)")