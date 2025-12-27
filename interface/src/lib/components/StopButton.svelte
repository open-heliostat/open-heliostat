<script lang="ts">
	import Stop from '~icons/tabler/hand-stop';
	import Button from './Button.svelte';
    export let onClick : () => void;
</script>

<Button
    label="Stop"
    {onClick}
    >
    <Stop class="mr-2 h-5 w-5" />
</Button>