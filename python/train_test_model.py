import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import classification_report, accuracy_score, f1_score
from tensorflow import keras
from tensorflow.keras import layers
import matplotlib.pyplot as plt

# === 1. Carregar dados ===
train_df = pd.read_csv("UNSW_NB15_training-set.csv")
test_df = pd.read_csv("UNSW_NB15_testing-set.csv")

# === 2. Remover colunas irrelevantes ou que vazam rótulo ===
train_df = train_df.drop(columns=["id", "attack_cat","ct_state_ttl"])
test_df = test_df.drop(columns=["id", "attack_cat","ct_state_ttl"])

# === 3. Separar entrada e rótulo ===
X_train = train_df.drop(columns=["label"])
y_train = train_df["label"]

X_test = test_df.drop(columns=["label"])
y_test = test_df["label"]

# === 4. Codificar categóricas ===
cat_cols = ["proto", "service", "state"]
X_train = pd.get_dummies(X_train, columns=cat_cols)
X_test = pd.get_dummies(X_test, columns=cat_cols)

# Alinhar colunas entre treino e teste
X_train, X_test = X_train.align(X_test, join="left", axis=1, fill_value=0)

# === 5. Normalizar ===
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# === 6. Função para criar modelo ===
def build_model(input_shape):
    model = keras.Sequential([
        keras.Input(shape=(input_shape,)),
        layers.Dense(64, activation="relu"),
        layers.Dense(32, activation="relu"),
        layers.Dense(1, activation="sigmoid")
    ])
    model.compile(optimizer="adam", loss="binary_crossentropy", metrics=["accuracy"])
    return model

# === 7. Treinar modelos com diferentes épocas ===
epochs_list = [20]
results = []

for epochs in epochs_list:
    print(f"\nTreinando modelo com {epochs} épocas...")
    model = build_model(X_train_scaled.shape[1])
    
    model.fit(
        X_train_scaled, y_train,
        epochs=epochs,
        batch_size=256,
        validation_split=0.2,
        verbose=1
    )
    
    y_pred_probs = model.predict(X_test_scaled)
    y_pred = (y_pred_probs > 0.5).astype(int)
    
    acc = accuracy_score(y_test, y_pred)
    f1 = f1_score(y_test, y_pred)
    
    results.append({
        "epochs": epochs,
        "accuracy": acc,
        "f1_score": f1
    })
    
    print(f"Acurácia: {acc:.4f} | F1-score: {f1:.4f}")

# === 8. Mostrar resultados ===
print("\n=== Comparação dos Modelos ===")
for r in results:
    print(f"{r['epochs']} épocas → Acurácia: {r['accuracy']*100:.2f}%, F1: {r['f1_score']:.4f}")

model.save("model20.keras")

# === 9. Plotar gráfico ===
epochs_vals = [r["epochs"] for r in results]
acc_vals = [r["accuracy"] for r in results]
f1_vals = [r["f1_score"] for r in results]

plt.figure(figsize=(10,5))
plt.plot(epochs_vals, acc_vals, marker='o', label="Acurácia")
plt.plot(epochs_vals, f1_vals, marker='s', label="F1-score")
plt.title("Desempenho por número de épocas")
plt.xlabel("Épocas")
plt.ylabel("Pontuação")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
