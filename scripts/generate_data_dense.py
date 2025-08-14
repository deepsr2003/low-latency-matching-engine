import csv
import random
import numpy as np

# --- Configuration ---
NUM_MESSAGES = 2_000_000
PRESEED_ORDERS = 50_000
ADD_RATIO = 0.55
OUTPUT_FILE = 'market_data_large.csv'

# Tightly clustered prices for a liquid market


def generate_price():
    return int(np.random.normal(loc=10000, scale=25))

# --- Main Generation Logic ---


def generate_data(filename, price_generator):
    print(f"Generating data for {filename}...")
    active_orders = []
    order_id_counter = 1

    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)

        # 1. Pre-seed the book
        for _ in range(PRESEED_ORDERS):
            side = random.choice(['B', 'S'])
            price = price_generator()
            quantity = random.randint(1, 100)
            writer.writerow(['A', side, order_id_counter, price, quantity])
            active_orders.append(order_id_counter)
            order_id_counter += 1

        # 2. Generate the main body of messages
        for i in range(NUM_MESSAGES):
            if (i % 200000 == 0):
                print(f"  ... {i / NUM_MESSAGES * 100:.0f}% complete")

            # Choose between Add and Cancel
            if random.random() < ADD_RATIO or not active_orders:
                # Add Order
                side = random.choice(['B', 'S'])
                price = price_generator()
                quantity = random.randint(1, 100)
                writer.writerow(['A', side, order_id_counter, price, quantity])
                active_orders.append(order_id_counter)
                order_id_counter += 1
            else:
                # Cancel Order
                order_to_cancel = random.choice(active_orders)
                active_orders.remove(order_to_cancel)
                # For cancel, side, price, qty are not strictly needed by the book
                # but we include them for consistent CSV structure.
                writer.writerow(['C', 'B', order_to_cancel, 0, 0])

    print(f"Finished generating {filename} with {
          order_id_counter - 1} total orders.")


if __name__ == '__main__':
    generate_data(OUTPUT_FILE, generate_price)
