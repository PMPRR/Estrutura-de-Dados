# Criado por IA Gemin, baseado no ódigo do fernando nishio

import pandas as pd
import numpy as np
from tensorflow import keras
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import classification_report, accuracy_score

# === 1. Carregar modelo treinado ===
model = keras.models.load_model("model20.keras")

# === 2. Carregar dados de teste ===
df = pd.read_csv("UNSW_NB15_testing-set.csv")

# === 3. Remover colunas que vazam o rótulo ===
df = df.drop(columns=["id", "attack_cat", "ct_state_ttl"])

# === 4. Separar entrada e rótulo verdadeiro ===
X = df.drop(columns=["label"])
y_true = df["label"]

# === 5. Codificar variáveis categóricas ===
cat_cols = ["proto", "service", "state"]
X = pd.get_dummies(X, columns=cat_cols)

# === 6. Recarregar os dados de treino para garantir alinhamento de colunas ===
X_treino_temp = pd.read_csv("UNSW_NB15_training-set.csv") \
    .drop(columns=["id", "attack_cat", "label", "ct_state_ttl"])
X_treino_temp = pd.get_dummies(X_treino_temp, columns=cat_cols)

# Alinhar com colunas usadas no treino
X = X.reindex(columns=X_treino_temp.columns, fill_value=0)

# === 7. Normalizar (refazendo scaler no novo conjunto) ===
scaler = StandardScaler()
X_scaled = scaler.fit(X).transform(X)

# === 8. Fazer predições ===
y_pred_probs = model.predict(X_scaled)
y_pred = (y_pred_probs > 0.5).astype(int)

# === 9. Avaliação ===
print("Relatório de classificação:")
print(classification_report(y_true, y_pred))
print(f"Acurácia: {accuracy_score(y_true, y_pred) * 100:.2f}%")

# === 10. Salvar predições ===
df["predito"] = y_pred
df[["label", "predito"]].to_csv("predicoes.csv", index=False)
print("Predições salvas em 'predicoes.csv'")
