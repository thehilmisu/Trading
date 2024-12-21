import matplotlib.pyplot as plt

# File path
file_path = "analysis.txt"

# Initialize lists
prices = []
labels = []
filtered_indices = []  # Indices where we decide to display the HOLD label

# Read and parse the file
with open(file_path, "r") as file:
    for i, line in enumerate(file):
        # Split the line by tabs
        parts = line.strip().split("\t")
        if len(parts) < 6:
            continue  # Skip invalid lines

        # Extract the price and label
        price = float(parts[3].split(":")[1])  # Extract the value after "Price:"
        label = parts[-1]  # Last part is the label

        # Append price
        prices.append(price)

        # Determine if we want to include this label
        if label == "HOLD" and i % 10 == 0:  # Include every 10th HOLD for clarity
            labels.append(label)
            filtered_indices.append(i)
        elif label != "HOLD":  # Always include other labels
            labels.append(label)
        else:
            labels.append("")  # Skip excessive HOLD labels for clarity

# Plot the data
plt.figure(figsize=(50, 25))
plt.plot(prices, marker="o", label="Price")

# Add labels for all non-empty ones
for idx, label in enumerate(labels):
    if label:  # Only plot labels that aren't empty
        plt.text(idx, prices[idx], label, fontsize=8, color="red", ha="center")

# Configure the plot
plt.title("Price Plot with Selective HOLD Labels")
plt.xlabel("Time (arbitrary units)")
plt.ylabel("Price")
plt.grid(True)
plt.legend()
plt.show()

