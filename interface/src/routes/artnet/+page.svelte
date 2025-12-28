<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import { page } from '$app/stores';
	import { user } from '$lib/stores/user';
	import SettingsCard from '$lib/components/SettingsCard.svelte';
	import Spinner from '$lib/components/Spinner.svelte';
	import { notifications } from '$lib/components/toasts/notifications';
	import Antenna from '~icons/tabler/antenna';
	import Refresh from '~icons/tabler/refresh';
	import Shield from '~icons/tabler/shield-lock';

	type ChannelConfig = {
		coarse: number;
		fine: number;
		min: number;
		max: number;
	};

	type ArtNetStatus = {
		listening: boolean;
		lastPacketMs: number;
		lastPacketAgeMs?: number;
		lastUniverse: number;
		lastSequence: number;
		lastAzimuth: number;
		lastElevation: number;
	};

	type ArtNetSettings = {
		enabled: boolean;
		takeoverHeliostat: boolean;
		universe: number;
		enableChannel: number;
		enableThreshold: number;
		azimuth: ChannelConfig;
		elevation: ChannelConfig;
		status: ArtNetStatus;
	};

	const defaultSettings: ArtNetSettings = {
		enabled: false,
		takeoverHeliostat: true,
		universe: 0,
		enableChannel: 0,
		enableThreshold: 10,
		azimuth: { coarse: 1, fine: 2, min: 0, max: 360 },
		elevation: { coarse: 3, fine: 4, min: 0, max: 90 },
		status: {
			listening: false,
			lastPacketMs: 0,
			lastUniverse: 0,
			lastSequence: 0,
			lastAzimuth: 0,
			lastElevation: 0
		}
	};

	let settings: ArtNetSettings = $state(structuredClone(defaultSettings));
	let loading = $state(true);
	let refreshTimer: ReturnType<typeof setInterval> | undefined;

	function authHeaders() {
		return {
			Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
			'Content-Type': 'application/json'
		};
	}

	async function fetchSettings() {
		try {
			const res = await fetch('/rest/artnet', { headers: authHeaders() });
			if (!res.ok) throw new Error(`GET failed: ${res.status}`);
			const data = await res.json();
			settings = normalizeSettings(data);
		} catch (err) {
			console.error(err);
			notifications.error('Failed to load Art-Net settings', 3000);
		} finally {
			loading = false;
		}
	}

	async function saveSettings() {
		try {
			const res = await fetch('/rest/artnet', {
				method: 'POST',
				headers: authHeaders(),
				body: JSON.stringify(serializeSettings(settings))
			});
			if (!res.ok) throw new Error(`POST failed: ${res.status}`);
			const data = await res.json();
			settings = normalizeSettings(data);
			notifications.success('Art-Net settings saved', 2500);
		} catch (err) {
			console.error(err);
			notifications.error('Failed to save Art-Net settings', 3000);
		}
	}

	function normalizeSettings(raw: any): ArtNetSettings {
		return {
			enabled: !!raw.enabled,
			takeoverHeliostat: !!raw.takeoverHeliostat,
			universe: Number(raw.universe) || 0,
			enableChannel: Number(raw.enableChannel) || 0,
			enableThreshold: Number(raw.enableThreshold) || 0,
			azimuth: {
				coarse: Number(raw.azimuth?.coarse) || 0,
				fine: Number(raw.azimuth?.fine) || 0,
				min: Number(raw.azimuth?.min ?? raw.azimuth?.minValue ?? 0),
				max: Number(raw.azimuth?.max ?? raw.azimuth?.maxValue ?? 0)
			},
			elevation: {
				coarse: Number(raw.elevation?.coarse) || 0,
				fine: Number(raw.elevation?.fine) || 0,
				min: Number(raw.elevation?.min ?? raw.elevation?.minValue ?? 0),
				max: Number(raw.elevation?.max ?? raw.elevation?.maxValue ?? 0)
			},
			status: {
				listening: !!raw.status?.listening,
				lastPacketMs: Number(raw.status?.lastPacketMs) || 0,
				lastPacketAgeMs: raw.status?.lastPacketAgeMs,
				lastUniverse: Number(raw.status?.lastUniverse) || 0,
				lastSequence: Number(raw.status?.lastSequence) || 0,
				lastAzimuth: Number(raw.status?.lastAzimuth) || 0,
				lastElevation: Number(raw.status?.lastElevation) || 0
			}
		};
	}

	function serializeSettings(cfg: ArtNetSettings) {
		return {
			enabled: cfg.enabled,
			takeoverHeliostat: cfg.takeoverHeliostat,
			universe: cfg.universe,
			enableChannel: cfg.enableChannel,
			enableThreshold: cfg.enableThreshold,
			azimuth: {
				coarse: cfg.azimuth.coarse,
				fine: cfg.azimuth.fine,
				min: cfg.azimuth.min,
				max: cfg.azimuth.max
			},
			elevation: {
				coarse: cfg.elevation.coarse,
				fine: cfg.elevation.fine,
				min: cfg.elevation.min,
				max: cfg.elevation.max
			}
		};
	}

	function updateNumber<T extends keyof ArtNetSettings>(key: T, value: number) {
		// @ts-expect-error - dynamic assignment on state object
		settings[key] = Number.isFinite(value) ? value : 0;
	}

	function updateChannel(target: ChannelConfig, field: keyof ChannelConfig, value: string) {
		const num = Number(value);
		target[field] = Number.isFinite(num) ? num : 0;
	}

	function formatLastPacket(): string {
		if (!settings.status.lastPacketMs) return 'No Art-Net packets received yet';
		const age = settings.status.lastPacketAgeMs ?? (Date.now() - settings.status.lastPacketMs);
		const seconds = Math.max(0, Math.floor(age / 1000));
		const az = settings.status.lastAzimuth.toFixed(1);
		const el = settings.status.lastElevation.toFixed(1);
		return `${seconds}s ago • universe ${settings.status.lastUniverse} • seq ${settings.status.lastSequence} • az ${az}° • el ${el}°`;
	}

	onMount(() => {
		fetchSettings();
		refreshTimer = setInterval(fetchSettings, 5000);
	});

	onDestroy(() => {
		if (refreshTimer) clearInterval(refreshTimer);
	});
</script>

<div class="mx-0 my-1 flex flex-col space-y-4 sm:mx-8 sm:my-8">
	<SettingsCard collapsible={false}>
		{#snippet icon()}
			<Antenna class="mr-2 h-6 w-6" />
		{/snippet}
		{#snippet title()}
			<span>Art-Net Control</span>
		{/snippet}
		<div class="flex flex-col gap-6">
			{#if loading}
				<div class="flex items-center gap-2 text-base">
					<Spinner />
					<span>Loading Art-Net settings…</span>
				</div>
			{:else}
				<div class="grid grid-cols-1 gap-4 lg:grid-cols-2">
					<div class="rounded-box border border-base-300 bg-base-100 p-4 shadow-sm">
						<div class="flex items-center justify-between">
							<div class="flex items-center gap-2 text-lg font-semibold">
								<Shield class="h-5 w-5" />
								<span>Service</span>
							</div>
							<button class="btn btn-sm" type="button" onclick={fetchSettings}>
								<Refresh class="h-4 w-4" />
								Refresh
							</button>
						</div>
						<div class="form-control mt-3">
							<label class="label cursor-pointer justify-between">
								<span class="label-text">Enable Art-Net listener</span>
								<input type="checkbox" class="toggle" bind:checked={settings.enabled} />
							</label>
						</div>
						<div class="form-control">
							<label class="label cursor-pointer justify-between">
								<span class="label-text">Take over heliostat</span>
								<input type="checkbox" class="toggle" bind:checked={settings.takeoverHeliostat} />
							</label>
							<p class="text-sm text-base-content/70">Disables automatic sun tracking while Art-Net DMX is active.</p>
						</div>
						<div class="form-control">
							<label class="label"><span class="label-text">Universe</span></label>
							<input
								type="number"
								class="input input-bordered"
								min="0"
								max="65535"
								value={settings.universe}
								oninput={(ev) => updateNumber('universe', Number((ev.target as HTMLInputElement).value))}
							/>
						</div>
						<div class="form-control">
							<label class="label"><span class="label-text">Enable channel (0 = disabled)</span></label>
							<input
								type="number"
								class="input input-bordered"
								min="0"
								max="512"
								value={settings.enableChannel}
								oninput={(ev) => updateNumber('enableChannel', Number((ev.target as HTMLInputElement).value))}
							/>
							<label class="label"><span class="label-text-alt">Threshold value to activate: {settings.enableThreshold}</span></label>
							<input
								type="range"
								class="range range-sm"
								min="0"
								max="255"
								step="1"
								value={settings.enableThreshold}
								oninput={(ev) => updateNumber('enableThreshold', Number((ev.target as HTMLInputElement).value))}
							/>
						</div>
					</div>

					<div class="rounded-box border border-base-300 bg-base-100 p-4 shadow-sm">
						<div class="flex items-center gap-2 text-lg font-semibold">
							<Refresh class="h-5 w-5" />
							<span>Status</span>
						</div>
						<div class="mt-3 space-y-2 text-sm">
							<div class="flex items-center justify-between">
								<span>Listening</span>
								<span class={settings.status.listening ? 'badge badge-success' : 'badge badge-ghost'}>
									{settings.status.listening ? 'Yes' : 'No'}
								</span>
							</div>
							<div class="flex items-center justify-between">
								<span>Last packet</span>
								<span class="text-base-content/80">{formatLastPacket()}</span>
							</div>
						</div>
					</div>
				</div>

				<div class="rounded-box border border-base-300 bg-base-100 p-4 shadow-sm">
					<div class="flex items-center gap-2 text-lg font-semibold">
						<Refresh class="h-5 w-5" />
						<span>Channel Mapping</span>
					</div>
					<div class="mt-3 grid grid-cols-1 gap-4 md:grid-cols-2">
						<div class="space-y-2">
							<h3 class="font-semibold">Azimuth</h3>
							<label class="label"><span class="label-text">Coarse channel</span></label>
							<input
								type="number"
								class="input input-bordered"
								min="0"
								max="512"
								value={settings.azimuth.coarse}
								oninput={(ev) => updateChannel(settings.azimuth, 'coarse', (ev.target as HTMLInputElement).value)}
							/>
							<label class="label"><span class="label-text">Fine channel (optional)</span></label>
							<input
								type="number"
								class="input input-bordered"
								min="0"
								max="512"
								value={settings.azimuth.fine}
								oninput={(ev) => updateChannel(settings.azimuth, 'fine', (ev.target as HTMLInputElement).value)}
							/>
							<label class="label"><span class="label-text">Angle range (deg)</span></label>
							<div class="grid grid-cols-2 gap-2">
								<input
									type="number"
									class="input input-bordered"
									min="-360"
									max="720"
									value={settings.azimuth.min}
									oninput={(ev) => updateChannel(settings.azimuth, 'min', (ev.target as HTMLInputElement).value)}
								/>
								<input
									type="number"
									class="input input-bordered"
									min="-360"
									max="720"
									value={settings.azimuth.max}
									oninput={(ev) => updateChannel(settings.azimuth, 'max', (ev.target as HTMLInputElement).value)}
								/>
							</div>
						</div>
						<div class="space-y-2">
							<h3 class="font-semibold">Elevation</h3>
							<label class="label"><span class="label-text">Coarse channel</span></label>
							<input
								type="number"
								class="input input-bordered"
								min="0"
								max="512"
								value={settings.elevation.coarse}
								oninput={(ev) => updateChannel(settings.elevation, 'coarse', (ev.target as HTMLInputElement).value)}
							/>
							<label class="label"><span class="label-text">Fine channel (optional)</span></label>
							<input
								type="number"
								class="input input-bordered"
								min="0"
								max="512"
								value={settings.elevation.fine}
								oninput={(ev) => updateChannel(settings.elevation, 'fine', (ev.target as HTMLInputElement).value)}
							/>
							<label class="label"><span class="label-text">Angle range (deg)</span></label>
							<div class="grid grid-cols-2 gap-2">
								<input
									type="number"
									class="input input-bordered"
									min="-360"
									max="720"
									value={settings.elevation.min}
									oninput={(ev) => updateChannel(settings.elevation, 'min', (ev.target as HTMLInputElement).value)}
								/>
								<input
									type="number"
									class="input input-bordered"
									min="-360"
									max="720"
									value={settings.elevation.max}
									oninput={(ev) => updateChannel(settings.elevation, 'max', (ev.target as HTMLInputElement).value)}
								/>
							</div>
						</div>
					</div>
				</div>

				<div class="flex justify-end gap-2">
					<button class="btn btn-ghost" type="button" onclick={() => (settings = structuredClone(defaultSettings))}>
						Reset
					</button>
					<button class="btn btn-primary" type="button" onclick={saveSettings}>
						Save Art-Net
					</button>
				</div>
			{/if}
		</div>
	</SettingsCard>
</div>
